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

#include <signal.h>
#include <assert.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "server.h"
#include "loader.h"
#include "dbus_service.h"
#include "project.h"
#include "client.h"
#include "appdb.h"
#include "file.h"
#include "client_dependency.h"
#include "dbus_iface_control.h"
#include "common/safety.h"
#include "common/debug.h"
#include "common/klist.h"

#ifdef HAVE_JACK_DBUS
# include "jackdbus_mgr.h"
#else
# include "jack_mgr.h"
#endif

server_t *g_server = NULL;

static void
server_fill_projects(server_t *server);

static void
server_destroy(server_t *server);

bool
server_start(const char *default_dir)
{
	server_t *server;

	server = lash_calloc(1, sizeof(server_t));

	INIT_LIST_HEAD(&server->loaded_projects);
	INIT_LIST_HEAD(&server->all_projects);

	lash_debug("Starting server");

	if (!lash_appdb_load(&server->appdb)) {
		lash_error("Failed to load application database");
		free(server);
		return false;
	}

#ifdef LASH_DEBUG
	struct list_head *node;
	struct lash_appdb_entry *entry;

	list_for_each (node, &server->appdb) {
		entry = list_entry(node, struct lash_appdb_entry, siblings);
		lash_info("Application '%s'", entry->name);
	}
#endif

	server->projects_dir = lash_dup_fqn(getenv("HOME"), default_dir);

	server_fill_projects(server);

#ifndef HAVE_JACK_DBUS
	server->jack_mgr = jack_mgr_new();
#endif
#ifdef HAVE_ALSA
	server->alsa_mgr = alsa_mgr_new();
#endif

	server->dbus_service = lashd_dbus_service_new(server);
	if (!server->dbus_service) {
		lash_debug("Failed to launch D-Bus service");
		goto fail;
	}
	g_server = server;

#ifdef HAVE_JACK_DBUS
	server->jackdbus_mgr = lashd_jackdbus_mgr_new(server);
	if (!server->jackdbus_mgr) {
		lash_debug("Failed to launch JACK D-Bus manager");
		goto fail;
	}
#endif

	lash_debug("Server running");

	return true;

fail:
	server_destroy(server);
	return false;
}

static void
server_destroy(server_t *server)
{
	struct list_head *node;
	project_t *project;

	while (!list_empty(&server->all_projects)) {
		node = server->all_projects.next;
		list_del(node);
		project = list_entry(node, project_t, siblings_all);
		project_destroy(project);
	}

	lash_debug("Destroying D-Bus service");
	service_destroy(server->dbus_service);
	server->dbus_service = NULL;

#ifdef HAVE_JACK_DBUS
	lash_debug("Destroying JACK D-Bus manager");
	lashd_jackdbus_mgr_destroy(server->jackdbus_mgr);
#else
	lash_debug("Destroying JACK manager");
	jack_mgr_lock(server->jack_mgr);
	jack_mgr_destroy(server->jack_mgr);
#endif

#ifdef HAVE_ALSA
	lash_debug("Destroying ALSA manager");
	alsa_mgr_lock(server->alsa_mgr);
	alsa_mgr_destroy(server->alsa_mgr);
#endif

	lash_free(&server->projects_dir);

	lash_debug("Destroying application database");
	lash_appdb_free(&server->appdb);

	lash_free(&server);

	lash_debug("Server destroyed");
}

void
server_stop(void)
{
	if (g_server) {
		server_destroy(g_server);
		g_server = NULL;
	}
}

static void
server_fill_projects(server_t *server)
{
	DIR *dir;
	struct dirent *dentry;
	project_t *project;

	lash_debug("Getting projects from directory '%s'", server->projects_dir);

	dir = opendir(server->projects_dir);
	if (!dir) {
		lash_error("Cannot open directory '%s': %s",
		           server->projects_dir, strerror(errno));
		return;
	}

	while ((dentry = readdir(dir))) {
		if (dentry->d_type != DT_DIR)
			continue;

		/* Skip . and .. */
		if (dentry->d_name[0] == '.')
			continue;

		project = project_new_from_disk(server->projects_dir, dentry->d_name);
		if (project)
			list_add_tail(&project->siblings_all,
			              &server->all_projects);
	}

	closedir(dir);
}

/*****************************
 ******* server stuff ********
 *****************************/

project_t *
server_find_project_by_name(server_t   *server,
                            const char *project_name)
{
	struct list_head *node;
	project_t *project;

	if (!project_name)
		return NULL;

	list_for_each (node, &server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);
		if (strcmp(project->name, project_name) == 0)
			return project;
	}

	return NULL;
}

static __inline__ client_t *
find_client_in_list_by_dbus_name(struct list_head *client_list,
                                 const char       *dbus_name)
{
	struct list_head *node;
	client_t *client;

	list_for_each (node, client_list) {
		client = list_entry(node, client_t, siblings);

		if (strcmp(client->dbus_name, dbus_name) == 0)
			return client;
	}

	return NULL;
}

static __inline__ client_t *
find_client_in_list_by_pid(struct list_head *client_list,
                           pid_t             pid)
{
	struct list_head *node;
	client_t *client;

	list_for_each (node, client_list) {
		client = list_entry(node, client_t, siblings);

		if (client->pid == pid)
			return client;
	}

	return NULL;
}

client_t *
server_find_client_by_dbus_name(server_t   *server,
                                const char *dbus_name)
{
	struct list_head *node;
	project_t *project;
	client_t *client;

	list_for_each (node, &server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);

		client = find_client_in_list_by_dbus_name(&project->clients,
		                                          dbus_name);
		if (client)
			return client;
	}

	return NULL;
}

client_t *
server_find_client_by_id(
	uuid_t id)
{
	struct list_head *node;
	project_t *project;
	client_t *client;

	list_for_each (node, &g_server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);

		client = project_get_client_by_id(&project->clients, id);
		if (client)
			return client;
	}

	return NULL;
}

client_t *
server_find_client_by_pid(server_t *server,
                          pid_t     pid)
{
	struct list_head *node;
	project_t *project;
	client_t *client;

	list_for_each (node, &server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);

		client = find_client_in_list_by_pid(&project->clients, pid);
		if (client)
			return client;
	}

	return NULL;
}

client_t *
server_find_lost_client_by_pid(
	server_t * server,
	pid_t pid)
{
	struct list_head *node;
	project_t *project;
	client_t *client;

	list_for_each (node, &server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);

		client = find_client_in_list_by_pid(&project->lost_clients, pid);
		if (client)
			return client;
	}

	return NULL;
}

static const char *
server_create_new_project_name(server_t   *server,
                               const char *suggestion)
{
	static char new_name[1034];
	const char *info_file;
	int i;

	if (!suggestion || !suggestion[0]) {
		suggestion = "New Project";
	} else if (strlen(suggestion) > 1023) {
		lash_error("Suggested project name is too long "
		           "(over 1023 characters excl. NUL)");
		return NULL;
	}

	strcpy(new_name, suggestion);

	/* Yes, arbitrary limits suck, but I suppose this one is acceptable */
	for (i = 2; i <= 1000000000; ++i) {
		if (!server_find_project_by_name(server, new_name)) {
			info_file = lash_get_fqn(lash_get_fqn(server->projects_dir,
			                                      new_name),
			                         PROJECT_INFO_FILE);
			if (access(info_file, F_OK) != 0)
				return new_name;
		}

		sprintf(new_name, "%s %d", suggestion, i);
	}

	/* You win the WTF prize of superb WTF */
	lash_error("Cannot create new project name: Tried 999 999 999 "
	           "different names but all were taken");

	return NULL;
}

static __inline__ project_t *
server_create_new_project(server_t   *server,
                          const char *name)
{
	project_t *project;

	lash_debug("Creating a new project");

	project = project_new();
	lash_strset(&project->name, server_create_new_project_name(server, name));

	lash_debug("The new project's name is '%s'", project->name);

	lash_strset(&project->directory,
	            lash_get_fqn(server->projects_dir, project->name));

	project_clear_id_dir(project);

	list_add_tail(&project->siblings_loaded, &server->loaded_projects);

	/* if we save the project later we will add it to available for loading list */
	INIT_LIST_HEAD(&project->siblings_all);

	lashd_dbus_signal_emit_project_appeared(project->name, project->directory);

	lash_info("Created project '%s'", project->name);

	return project;
}

project_t *
server_get_newborn_project(
	server_t * server_ptr)
{
	struct list_head * node_ptr;
	project_t * project_ptr;

	project_ptr = NULL;

	list_for_each(node_ptr, &server_ptr->loaded_projects)
	{
		project_ptr = list_entry(node_ptr, project_t, siblings_loaded);
		if (project_ptr->doc == NULL)
		{
			return project_ptr;
		}
	}

	/* no unsaved project found */

	if (project_ptr != NULL)
	{
		/* reuse last loaded project if any */
		return project_ptr;
	}

	lash_debug("Creating new project for client");

	return server_create_new_project(server_ptr, NULL);
}

void
server_close_project(server_t  *server,
                     project_t *project)
{
	project_unload(project);

	lashd_dbus_signal_emit_project_disappeared(project->name);

	lash_info("Project '%s' removed", project->name);
}

bool
server_project_close_by_name(server_t   *server,
                             const char *project_name)
{
	project_t *project;

	project = server_find_project_by_name(server, project_name);
	if (!project)
		return false;

	server_close_project(server, project);

	return true;
}

void
server_close_all_projects(server_t *server)
{
	struct list_head *node;
	project_t *project;

	while (!list_empty(&server->loaded_projects)) {
		node = server->loaded_projects.next;
		project = list_entry(node, project_t, siblings_loaded);

		server_close_project(server, project); /* Removes from list */
	}
}

void
server_save_all_projects(server_t *server)
{
	struct list_head *node;
	project_t *project;

	list_for_each (node, &server->loaded_projects) {
		project = list_entry(node, project_t, siblings_loaded);
		project_save(project);
	}
}

client_t *
server_add_client(server_t    *server,
                  const char  *dbus_name,
                  pid_t        pid,
                  const char  *class,
                  int          flags,
                  const char  *working_dir,
                  int          argc,
                  char       **argv)
{
	client_t *client;

	/* See if we launched this client */
	if (pid && (client = server_find_lost_client_by_pid(server, pid)))
		goto resume;

	project_t *project = server_get_newborn_project(server);

	/* See if this is a recovering client */
	if (!(flags & LASH_No_Autoresume) && class
	    && (client = project_find_lost_client_by_class(project, class))) {
		client->pid = pid;
	resume:
		lash_strset(&client->dbus_name, dbus_name);
		client_resume_project(client);

	/* Otherwise add a new client */
	} else {
		client = client_new();
		client->pid = pid;
		lash_strset(&client->class, class);
		client->flags = flags;
		client->argc = argc;
		client->argv = argv;
		lash_strset(&client->dbus_name, dbus_name);
		lash_strset(&client->working_dir, working_dir);
		project_new_client(project, client);
	}

#ifdef HAVE_JACK_DBUS
	/* Try to find an unknown JACK client whose PID matches the newly
	   added LASH client's, if succesful bind them together */
	jack_mgr_client_t *jack_client;
	if ((jack_client = jack_mgr_client_find_by_pid(&(server->jackdbus_mgr->unknown_clients), pid)))
		lashd_jackdbus_mgr_bind_client(jack_client, client);
#endif

	return client;
}

static bool
server_project_restore(server_t  *server,
                       project_t *project)
{
	struct list_head *node;
	client_t *client;

	lash_info("Restoring project '%s'", project->name);

	if (project_is_loaded(project)) {
		lash_error("Cannot restore project: '%s' is already loaded", project->name);
		return false;
	}

	if (!project_load(project)) {
		lash_error("Cannot restore project '%s' from directory %s",
		           project->name, project->directory);
		return false;
	}

	// TODO: Shouldn't this check server->all_projects instead?
	if (server_find_project_by_name(server, project->name)) {
		const char *new_name = server_create_new_project_name(server,
		                                                      project->name);
		lash_info("Changing restored project's name from '%s' to '%s' "
		          "to avoid clashing with an existing project",
		          project->name, new_name);
		project_rename(project, new_name);
	}

	lash_info("Adding project '%s' to project list", project->name);
	list_add_tail(&project->siblings_loaded, &server->loaded_projects);

	lashd_dbus_signal_emit_project_appeared(project->name, project->directory);

	project->task_type = LASH_TASK_LOAD;
	project->client_tasks_total = project->client_tasks_progress = 0;

	list_for_each (node, &project->lost_clients) {
		client = list_entry(node, client_t, siblings);
		client->project = project;

		++project->client_tasks_total;

		/* Remove bogus entries from the client's dependencies list */
		client_dependency_list_sanity_check(&project->lost_clients,
		                                    client);

		/* Initialize the client's unsatisfied dependencies list */
		client_dependency_init_unsatisfied(client);

		/* First only launch clients which don't depend on others */
		if (!list_empty(&client->unsatisfied_deps)) {
			lash_debug("Postponing adding of client %s because "
			           "it depends on other clients",
			           client->id_str);
			continue;
		}

		project_launch_client(project, client);
	}

	project->client_tasks_pending = project->client_tasks_total;

	return true;
}

bool
server_project_restore_by_name(server_t   *server,
                               const char *project_name)
{
	struct list_head *node;
	project_t *project;

	list_for_each (node, &server->all_projects) {
		project = list_entry(node, project_t, siblings_all);

		if (strcmp(project->name, project_name) == 0)
			return server_project_restore(server, project);
	}

	return false;
}

bool
server_project_restore_by_dir(server_t   *server,
                              const char *directory)
{
	struct list_head *node;
	project_t *project;

	list_for_each (node, &server->all_projects) {
		project = list_entry(node, project_t, siblings_all);

		if (strcmp(project->directory, directory) == 0)
			return server_project_restore(server, project);
	}

	return false;
}

bool
server_project_save_by_name(server_t   *server,
                            const char *project_name)
{
	project_t *project;

	project = server_find_project_by_name(server, project_name);
	if (!project)
		return false;

	project_save(project);

	return true;
}

void
server_main(void)
{
	while (!g_server->quit) {
		dbus_connection_read_write_dispatch(g_server->dbus_service->connection, 50);

		loader_run();
		// TODO: wtf?
		loader_run();
	}

	lash_debug("Finished");
}

/* EOF */
