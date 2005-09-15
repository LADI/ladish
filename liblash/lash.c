/*
 *   LASH
 *    
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
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

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
extern int h_errno;

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/param.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

lash_args_t *
lash_extract_args(int *argc, char ***argv)
{
	int i, j, valid_count;
	lash_args_t *args;

	args = lash_args_new();

	for (i = 1; i < *argc; ++i) {
		if (strncasecmp("--lash-server=", (*argv)[i], 14) == 0) {
			LASH_DEBUGARGS("setting server from command line: '%s'",
						   (*argv)[i] + 14);
			lash_args_set_server(args, (*argv)[i] + 14);
			(*argv)[i] = NULL;
			continue;
		}

		if (strncasecmp("--lash-project=", (*argv)[i], 15) == 0) {
			LASH_DEBUGARGS("setting project from command line: '%s'",
						   (*argv)[i] + 15);
			lash_args_set_project(args, (*argv)[i] + 15);
			(*argv)[i] = NULL;
			continue;
		}

		if (strncmp("--lash-id=", (*argv)[i], 10) == 0) {
			uuid_t id;

			LASH_DEBUGARGS("setting id from command line: '%s'",
						   (*argv)[i] + 10);

			j = uuid_parse((*argv)[i] + 10, id);

			LASH_PRINT_DEBUG("id parsed");

			(*argv)[i] = NULL;

			if (j == -1) {
				fprintf(stderr,
						"%s: ERROR PARSING ID FROM COMMAND LINE!  THIS IS BAD!\n",
						__FUNCTION__);
			} else {
				lash_args_set_id(args, id);
			}
			continue;
		}

		if (strncmp("--lash-no-autoresume", (*argv)[i], 20) == 0) {
			LASH_PRINT_DEBUG
				("setting LASH_No_Autoresume flag from command line");

			lash_args_set_flag(args, LASH_No_Autoresume);

			(*argv)[i] = NULL;

			continue;
		}
	}

	LASH_PRINT_DEBUG("args checked");

	/* sort out the argv pointers */
	valid_count = *argc;
	for (i = 1; i < valid_count; ++i) {
		LASH_DEBUGARGS("checking argv[%d]", i);
		if ((*argv)[i] == NULL) {

			for (j = i; j < *argc - 1; ++j)
				(*argv)[j] = (*argv)[j + 1];

			valid_count--;
			i--;
		}
	}

	LASH_PRINT_DEBUG("done");

	*argc = valid_count;

	lash_args_set_args(args, *argc, (const char *const *)*argv);

	return args;
}

lash_client_t *
lash_init(lash_args_t * args,
		  const char *class, int client_flags, lash_protocol_t protocol)
{
	lash_connect_params_t *connect_params;
	lash_client_t *client;
	int err;
	char *str;
	const char *cstr;
	char wd[MAXPATHLEN];

	client = lash_client_new();
	connect_params = lash_connect_params_new();

	client->args = args;
	client->args->flags |= client_flags;
	lash_client_set_class(client, class);

	str = getcwd(wd, MAXPATHLEN);
	if (!str) {
		fprintf(stderr, "%s: could not get current working directory: %s\n",
				__FUNCTION__, strerror(errno));
		str = getenv("PWD");
		if (str)
			lash_connect_params_set_working_dir(connect_params, str);
		else
			lash_connect_params_set_working_dir(connect_params,
												getenv("HOME"));
	} else {
		lash_connect_params_set_working_dir(connect_params, str);
	}

	LASH_DEBUGARGS("protocol version for connect: %s",
				   lash_protocol_string(protocol));
	connect_params->protocol_version = protocol;
	connect_params->flags = args->flags;
	lash_connect_params_set_project(connect_params, args->project);
	lash_connect_params_set_class(connect_params, class);
	uuid_copy(connect_params->id, args->id);
	connect_params->argc = args->argc;
	connect_params->argv = args->argv;

	/* try and connect to the server */
	LASH_PRINT_DEBUG("connecting to server");
	cstr = lash_args_get_server(args);
	err = lash_comm_connect_to_server(client,
									  cstr ? cstr : "localhost",
									  "lash", connect_params);
	lash_connect_params_destroy(connect_params);
	if (err) {
		fprintf(stderr,
				"%s: could not connect to server '%s' - disabling lash\n",
				__FUNCTION__, cstr ? cstr : "localhost");
		lash_client_destroy(client);
		return NULL;
	}
	LASH_PRINT_DEBUG("connected to server");
	client->server_connected = 1;

	err =
		pthread_create(&client->recv_thread, NULL, lash_comm_recv_run,
					   client);
	if (err) {
		fprintf(stderr,
				"%s: error creating recieve thread - disabling lash: %s\n",
				__FUNCTION__, strerror(err));

		lash_client_destroy(client);
		return NULL;
	}

	err =
		pthread_create(&client->send_thread, NULL, lash_comm_send_run,
					   client);
	if (err) {
		fprintf(stderr,
				"%s: error creating send thread - disabling lash: %s\n",
				__FUNCTION__, strerror(err));

		client->recv_close = 1;
		pthread_join(client->recv_thread, NULL);

		lash_client_destroy(client);
		return NULL;
	}

	return client;
}

unsigned int
lash_get_pending_event_count(lash_client_t * client)
{
	unsigned int count = 0;

	if (!client)
		return 0;

	pthread_mutex_lock(&client->events_in_lock);

	if (client->events_in)
		count = lash_list_length(client->events_in);

	pthread_mutex_unlock(&client->events_in_lock);

	return count;
}

unsigned int
lash_get_pending_config_count(lash_client_t * client)
{
	unsigned int count = 0;

	if (!lash_enabled(client))
		return 0;

	pthread_mutex_lock(&client->configs_in_lock);

	if (client->events_in)
		count = lash_list_length(client->configs_in);

	pthread_mutex_unlock(&client->configs_in_lock);

	return count;
}

lash_event_t *
lash_get_event(lash_client_t * client)
{
	lash_event_t *event = NULL;

	if (!client)
		return NULL;

	pthread_mutex_lock(&client->events_in_lock);

	if (client->events_in) {
		event = (lash_event_t *) client->events_in->data;
		client->events_in = lash_list_remove(client->events_in, event);
	}

	pthread_mutex_unlock(&client->events_in_lock);

	return event;
}

lash_config_t *
lash_get_config(lash_client_t * client)
{
	lash_config_t *config = NULL;

	if (!client)
		return NULL;

	pthread_mutex_lock(&client->configs_in_lock);

	if (client->configs_in) {
		config = (lash_config_t *) client->configs_in->data;
		client->configs_in = lash_list_remove(client->configs_in, config);
	}

	pthread_mutex_unlock(&client->configs_in_lock);

	return config;
}

void
lash_send_comm_event(lash_client_t * client, lash_comm_event_t * event)
{
	pthread_mutex_lock(&client->comm_events_out_lock);
	client->comm_events_out =
		lash_list_append(client->comm_events_out, event);
	pthread_mutex_unlock(&client->comm_events_out_lock);
	pthread_cond_signal(&client->send_conditional);
}

void
lash_send_event(lash_client_t * client, lash_event_t * event)
{
	lash_comm_event_t *comm_event;

	if (!lash_enabled(client)) {
		lash_event_destroy(event);
		return;
	}

	comm_event = lash_malloc(sizeof(lash_comm_event_t));
	comm_event->type = LASH_Comm_Event_Event;
	comm_event->event_data.event = event;

	lash_send_comm_event(client, comm_event);
}

void
lash_send_config(lash_client_t * client, lash_config_t * config)
{
	lash_comm_event_t *comm_event;

	if (!lash_enabled(client)) {
		lash_config_destroy(config);
		return;
	}

	comm_event = lash_malloc(sizeof(lash_comm_event_t));
	comm_event->type = LASH_Comm_Event_Config;
	comm_event->event_data.config = config;

	lash_send_comm_event(client, comm_event);
}

const char *
lash_get_server_name(lash_client_t * client)
{
	if (!lash_enabled(client))
		return NULL;

	return lash_lookup_peer_name(client->socket);
}

int
lash_server_connected(lash_client_t * client)
{
	if (!client)
		return 0;

	return client->server_connected;
}

void
lash_jack_client_name(lash_client_t * client, const char *name)
{
	lash_event_t *event;

	if (!lash_enabled(client))
		return;

	if (!name)
		return;

	event = lash_event_new_with_all(LASH_Jack_Client_Name, name);

	lash_send_event(client, event);
}

void
lash_alsa_client_id(lash_client_t * client, unsigned char id)
{
	lash_event_t *event;

	if (!lash_enabled(client))
		return;

	if (!id)
		return;

	event = lash_event_new();
	lash_event_set_alsa_client_id(event, id);

	lash_send_event(client, event);
}

/* EOF */
