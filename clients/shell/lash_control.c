/*
 *   LASH 
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "common/safety.h"

#undef HAVE_CONFIG_H

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else	/* !defined(HAVE_READLINE_H) */
extern char *readline();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
/* no readline */
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else	/* !defined(HAVE_HISTORY_H) */
extern void add_history();
extern int write_history();
extern int read_history();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
/* no history */
#endif /* HAVE_READLINE_HISTORY */

#include "lash/lash.h"
#include "common/debug.h"

#include "lash_control.h"
#include "client.h"

void
lash_control_init(lash_control_t * control)
{
	control->cur_project = NULL;
	control->client = NULL;
	control->projects = NULL;

	rl_initialize();
	rl_prep_terminal(1);
}

void
lash_control_free(lash_control_t * control)
{
	lash_list_t *node;

	for (node = control->projects; node; node = lash_list_next(node))
		project_destroy((project_t *) node->data);

	lash_list_free(control->projects);

	rl_deprep_terminal();
}

static void
command_help()
{
	printf("\n");
	printf("  list              List projects\n");
	printf("  project <name>    Select the project with name <name>\n");
	printf
		("  dir <dir>         Set the selected project's directory to <dir>\n");
	printf("  name <name>       Set the selected project's name to <name>\n");
	printf("  remove <client>   Remove <client> from the selected project\n");
	printf("  save              Save the selected project\n");
	printf("  restore <dir>     Restore a project from <dir>\n");
	printf("  close             Close the selected project\n");
/*  printf ("  tar  <tarball>    Pack the selected project into <tarball>\n");
  printf ("  untar <tarball>   Unpack and restore a project from <tarball>\n");*/
	printf("  quit              Exit the program\n");
	printf("\n");
	printf
		("  It is only necessary to type enough characters of a command to\n");
	printf("  disambiguate it from any other command\n");
	printf("\n");
}

static void
command_list(lash_control_t * control)
{
	lash_list_t *pnode, *clnode;
	project_t *project;
	client_t *client;
	char client_jack_str[64];
	char client_alsa_str[32];

	if (!control->projects) {
		printf("  No projects\n");
		return;
	}

	for (pnode = control->projects; pnode; pnode = lash_list_next(pnode)) {
		project = (project_t *) pnode->data;

		printf("  %s in directory %s, %s\n",
			   project->name,
			   project->dir, project->clients ? "clients:" : "no clients");

		for (clnode = project->clients; clnode;
			 clnode = lash_list_next(clnode)) {
			client = (client_t *) clnode->data;

			if (!client->alsa_client_id)
				client_alsa_str[0] = '\0';
			else
				sprintf(client_alsa_str, ", ALSA client ID: %u",
						(unsigned int)client->alsa_client_id);

			if (!client->jack_client_name)
				client_jack_str[0] = '\0';
			else
				sprintf(client_jack_str, ", JACK client name: %s",
						client->jack_client_name);

			printf("    %s%s%s\n", client_get_identity(client),
				   client_jack_str, client_alsa_str);
		}
	}
}

static void
command_project(lash_control_t * control, char *arg)
{
	lash_list_t *node;
	project_t *project;

	if (!arg || !strlen(arg)) {
		if (control->cur_project)
			printf("  Selected project: %s\n", control->cur_project->name);
		else
			printf("  No project selected\n");
		return;
	}

	for (node = control->projects; node; node = lash_list_next(node)) {
		project = (project_t *) node->data;

		if (strcmp(project->name, arg) == 0) {
			control->cur_project = project;
			return;
		}
	}

	printf("  Unknown project '%s'\n", arg);
}

static void
command_dir(lash_control_t * control, char *arg)
{
	size_t len;

	if (!control->cur_project) {
		printf("  No project selected\n");
		return;
	}

	if (!arg) {
		printf("  %s\n", control->cur_project->dir);
		return;
	}

	len = strlen(arg);
	if (arg[len - 1] == '/')
		arg[len - 1] = '\0';

	lash_control_move_project(control->client, control->cur_project->name, arg);
	printf("  Told server to change directory to %s\n", arg);
}

static void
command_remove(lash_control_t * control, char *arg)
{
/*
	lash_event_t *event;
	lash_list_t *node;
	client_t *client;

	if (!control->cur_project) {
		printf("  No project selected\n");
		return;
	}

	if (!arg) {
		printf("  Command requires an argument\n");
		return;
	}

	for (node = control->cur_project->clients; node;
		 node = lash_list_next(node)) {
		client = (client_t *) node->data;

		if (strcmp(client_get_identity(client), arg) == 0)
			break;
	}

	if (!node) {
		printf("  Unknown client '%s' in project\n", arg);
		return;
	}

	event = lash_event_new_with_type(LASH_Client_Remove);
	lash_event_set_project(event, control->cur_project->name);
	lash_event_set_client_id(event, client->id);
	lash_send_event(control->client, event);

	printf("  Told server to remove client '%s'\n", arg);
*/
}

static void
command_restore(lash_control_t * control, char *arg)
{
	if (!arg) {
		printf("  Command requires an argument\n");
		return;
	}

	lash_control_load_project_path(control->client, arg);
	printf("  Told server to restore project in dir '%s'\n", arg);
}

static void
command_name(lash_control_t * control, char *arg)
{
	if (!control->cur_project) {
		printf("  No project selected\n");
		return;
	}

	if (!arg) {
		printf("  Command requires an argument\n");
		return;
	}

	lash_control_name_project(control->client, control->cur_project->name, arg);
	printf("  Told server to set project name to '%s'\n", arg);
}

static void
command_save(lash_control_t * control)
{
	if (!control->cur_project) {
		printf("  No project selected\n");
		return;
	}

	lash_control_save_project(control->client, control->cur_project->name);
	printf("  Told server to save project\n");
}

static void
command_close(lash_control_t * control)
{
	if (!control->cur_project) {
		printf("  No project selected\n");
		return;
	}

	lash_control_close_project(control->client, control->cur_project->name);
	printf("  Told server to close project\n");
}

static int
deal_with_command(lash_control_t * control, char *command)
{
	char *ptr;
	char *arg;

	while (isspace(*command))
		command++;

	arg = strchr(command, ' ');
	if (arg) {
		while (isspace(*arg) && *arg != '\0')
			arg++;

		if (*arg == '\0')
			arg = NULL;
	}

	if (strncasecmp(command, "help", 1) == 0) {
		command_help();
		return 0;
	}

	if (strncasecmp(command, "list", 1) == 0) {
		command_list(control);
		return 0;
	}

	if (strncasecmp(command, "project", 1) == 0) {
		command_project(control, arg);
		return 0;
	}

	if (strncasecmp(command, "dir", 1) == 0) {
		command_dir(control, arg);
		return 0;
	}

	if (strncasecmp(command, "name", 1) == 0) {
		command_name(control, arg);
		return 0;
	}

	if (strncasecmp(command, "remove", 3) == 0) {
		command_remove(control, arg);
		return 0;
	}

	if (strncasecmp(command, "save", 1) == 0) {
		command_save(control);
		return 0;
	}

	if (strncasecmp(command, "restore", 3) == 0) {
		command_restore(control, arg);
		return 0;
	}

	if (strncasecmp(command, "close", 1) == 0) {
		command_close(control);
		return 0;
	}

	if (strncasecmp(command, "quit", 1) == 0) {
		return 1;
	}

	ptr = strchr(command, ' ');
	if (ptr)
		*ptr = '\0';
	printf("unknown command '%s'\n", command);
	if (ptr)
		*ptr = ' ';

	return 0;
}

static project_t *
find_project_by_name(lash_control_t *control,
                     const char     *name)
{
	lash_list_t *node;
	project_t *project;
	for (node = control->projects; node; node = lash_list_next(node)) {
		project = (project_t *) node->data;
		if (strcmp(name, project->name) == 0)
			return project;
	}
	return NULL;
}

static client_t *
find_project_client_by_id(project_t *project,
                          uuid_t     id)
{
	lash_list_t *node;
	client_t *client;
	for (node = project->clients; node; node = lash_list_next(node)) {
		client = (client_t *) node->data;
		if (uuid_compare(client->id, id) == 0)
			return client;
	}
	return NULL;
}

static client_t *
find_client_by_id(lash_control_t *control,
                  uuid_t          id)
{
	lash_list_t *node;
	client_t *client;
	for (node = control->projects; node; node = lash_list_next(node)) {
		if ((client = find_project_client_by_id((project_t *) node->data, id)))
			return client;
	}
	return NULL;
}

void
lash_control_cb(enum LASH_Event_Type  type,
                const char           *str1,
                const char           *str2,
                uuid_t                client_id,
                void                 *data)
{
	project_t *project;
	client_t *client;
	lash_control_t *control = data;

	switch (type) {
	case LASH_Project_Dir:
		if ((project = find_project_by_name(control, str1))) {
			printf("* Project '%s' changed to directory %s\n", str1, str2);
			lash_strset(&project->dir, str2);
		}
		break;

	case LASH_Project_Name:
		if ((project = find_project_by_name(control, str1))) {
			printf("* Project '%s' changed name to '%s'\n", str1, str2);
			lash_strset(&project->name, str2);
		}
		break;

	case LASH_Project_Add:
		printf("* New project '%s'\n", str1);
		project_t *new_project = project_new(str1);
		control->projects = lash_list_append(control->projects, new_project);
		/* Set the current project if it's the only one - save user's wrists! */
		if (lash_list_length(control->projects) == 1)
			control->cur_project = new_project;
		break;

	case LASH_Project_Remove:
		if ((project = find_project_by_name(control, str1))) {
			control->projects = lash_list_remove(control->projects, project);
			if (control->cur_project == project)
				control->cur_project = NULL;
			printf("* Project '%s' removed\n", project->name);
			project_destroy(project);
		}
		break;

	case LASH_Client_Add:
		if ((project = find_project_by_name(control, str1))) {
			client = client_new(client_id);
			printf("* New client '%s' in project '%s'\n",
			       client_get_identity(client), project->name);
			project->clients = lash_list_append(project->clients, client);
		}
		break;

	case LASH_Client_Remove:
		if (!uuid_is_null(client_id)
		    && (project = find_project_by_name(control, str1))
		    && (client = find_project_client_by_id(project, client_id))) {
			printf("* Client '%s' removed from project '%s'\n",
			       client_get_identity(client), project->name);
			project->clients = lash_list_remove(project->clients, client);
			client_destroy(client);
		}
		break;

	case LASH_Client_Name:
		if (!uuid_is_null(client_id)
		    && (client = find_client_by_id(control, client_id))) {
			printf("* Client '%s' changed its name to '%s'\n",
			       client_get_identity(client), str1);
			lash_strset(&client->name, str1);
		}
		break;

	case LASH_Jack_Client_Name:
		if (!uuid_is_null(client_id)
		    && (client = find_client_by_id(control, client_id))) {
			printf("* Client '%s' set its JACK client name to '%s'\n",
			       client_get_identity(client), str1);
			lash_strset(&client->jack_client_name, str1);
		}
		break;

	case LASH_Alsa_Client_ID:
		if (!uuid_is_null(client_id)
		    && (client = find_client_by_id(control, client_id))) {
			printf("* Client '%s' set its ALSA client ID to '%u'\n",
			       client_get_identity(client), lash_str_get_alsa_client_id(str1));
			client->alsa_client_id = lash_str_get_alsa_client_id(str1);
		}
		break;

	case LASH_Percentage:
		switch ((int) str1[0]) {
		case 0:
			printf("* Operation commencing\n");
			break;
		case 100:
			printf("* Operation complete\n");
			break;
		default:
			printf("* Operation %d%% complete\n", (int) str1[0]);
			break;
		}
		break;

	default:
		break;
	}
}

void
lash_control_main(lash_control_t * control)
{
	char *prompt;
	size_t prompt_size = 32;
	size_t project_prompt_size;
	char *command;
	int done = 0;

	prompt = lash_malloc(1, prompt_size);

	printf("\nLASH Control %s\n\n", PACKAGE_VERSION);

#ifdef HAVE_READLINE_HISTORY
	using_history();
#endif

	while (!done) {
		if (control->cur_project) {
			project_prompt_size = strlen(control->cur_project->name) + 4;

			if (project_prompt_size > prompt_size) {
				prompt_size = project_prompt_size;
				prompt = lash_realloc(prompt, 1, prompt_size);
			}

			sprintf(prompt, "%s > ", control->cur_project->name);
		} else
			sprintf(prompt, "(no project) > ");

		command = readline(prompt);

		if (!command) {
			printf("\n");
			return;
		}

		lash_dispatch(control->client);

		if (strlen(command)) {
			done = deal_with_command(control, command);

#ifdef HAVE_READLINE_HISTORY
			add_history(command);
#endif
		}

		free(command);
	}

}

/* EOF */
