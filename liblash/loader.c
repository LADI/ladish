/*
 *   LASH
 *    
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

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <lash/lash.h>
#include <lash/loader.h>
#include <lash/internal_headers.h>

#define XTERM_COMMAND_EXTENSION "&& sh || sh"
#define LASH_SERVER_OPTION    "--lash-server"
#define LASH_PROJECT_OPTION   "--lash-project"
#define LASH_ID_OPTION        "--lash-id"

loader_t *
loader_new()
{
	loader_t *loader;
	int sockets[2];
	int err;

	loader = lash_malloc0(sizeof(loader_t));
	loader->loader_pid = -1;

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		fprintf(stderr, "%s: could not create unix socket pair: %s\n",
				__FUNCTION__, strerror(errno));
		free(loader);
		return NULL;
	}

	loader->server_socket = sockets[0];
	loader->loader_socket = sockets[1];

	return loader;
}

void
loader_destroy(loader_t * loader)
{
	lash_comm_event_t event;
	int err;

	LASH_PRINT_DEBUG("sending close event to loader");
	lash_comm_event_set_type(&event, LASH_Comm_Event_Close);

	err = lash_comm_send_event(loader->server_socket, &event);
	if (err < 0) {
		fprintf(stderr,
				"%s: error sending Close event to loader; a dangling child process is likely\n",
				__FUNCTION__);
	}

	LASH_PRINT_DEBUG("close event sent");

	err = close(loader->server_socket);
	if (err == -1) {
		fprintf(stderr, "%s: error closing loader's server-side socket: %s\n",
				__FUNCTION__, strerror(errno));
	}

	free(loader);
}

/* all args are enclosed in quotes */
static size_t
get_command_size(int argc, char **argv)
{
	size_t size = 0;
	int i;

	for (i = 0; i < argc; i++)
		size += strlen(argv[i]) + 3;

	size += strlen(XTERM_COMMAND_EXTENSION);

	return size + 1;
}

static void
create_command(char *command_buffer, int argc, char **argv)
{
	char *ptr;
	int i;

	ptr = command_buffer;

	for (i = 0; i < argc; i++) {
		sprintf(ptr, "\"%s\" ", argv[i]);
		ptr += strlen(ptr);
	}

	sprintf(ptr, "%s", XTERM_COMMAND_EXTENSION);
}

static void
loader_exec_program_in_xterm(int argc, char **argv)
{
	size_t command_size;
	char *command_buffer;
	char *xterm_argv[6];

	command_size = get_command_size(argc, argv);

	command_buffer = lash_malloc(command_size);

	create_command(command_buffer, argc, argv);

	xterm_argv[0] = "xterm";
	xterm_argv[1] = "-e";
	xterm_argv[2] = "bash";
	xterm_argv[3] = "-c";
	xterm_argv[4] = command_buffer;
	xterm_argv[5] = NULL;

	/* execute it */
	execvp("xterm", xterm_argv);

	fprintf(stderr, "%s: execing program '%s' in an xterm failed: %s\n",
			__FUNCTION__, command_buffer, strerror(errno));

	exit(1);
}

static void
loader_exec_program(loader_t * loader, lash_exec_params_t * params)
{
	int err;
	char *project_opt;
	char *server_opt;
	char id_opt[sizeof(LASH_ID_OPTION) + 36];
	char **argv;
	int argc = 0;
	int i;

	/* no longer anything to do with lashd */
	err = setsid();
	if (err == -1) {
		fprintf(stderr, "%s: could not create new process group: %s",
				__FUNCTION__, strerror(errno));
	}

	/* change the working dir */
	err = chdir(params->working_dir);
	if (err == -1) {
		fprintf(stderr,
				"%s: could not change directory to working dir '%s' for program '%s': %s\n",
				__FUNCTION__, params->working_dir, params->argv[0],
				strerror(errno));
	}

	/* construct the args */
	project_opt =
		lash_malloc(strlen(params->project) + 1 +
					strlen(LASH_PROJECT_OPTION) + 1);
	sprintf(project_opt, "%s=%s", LASH_PROJECT_OPTION, params->project);

	server_opt =
		lash_malloc(strlen(params->server) + 1 + strlen(LASH_SERVER_OPTION) +
					1);
	sprintf(server_opt, "%s=%s", LASH_SERVER_OPTION, params->server);

	sprintf(id_opt, "%s=", LASH_ID_OPTION);
	uuid_unparse(params->id, id_opt + strlen(LASH_ID_OPTION) + 1);

	argc += params->argc + 3;
	argv = lash_malloc(sizeof(char *) * (argc + 1));

	for (i = 0; i < params->argc; i++)
		argv[i] = params->argv[i];

	argv[(i++)] = project_opt;
	argv[(i++)] = server_opt;
	argv[(i++)] = id_opt;
	argv[i] = NULL;

#ifdef LASH_DEBUG
	{
		int l;

		printf("%s: executing command: ", __FUNCTION__);
		for (l = 0; argv[l]; l++) {
			printf("%s ", argv[l]);
		}
		putchar('\n');
	}
#endif

	LASH_DEBUGARGS("flags: %d", params->flags);
	if (params->flags & LASH_Terminal) {
		LASH_PRINT_DEBUG("execing in terminal");
		loader_exec_program_in_xterm(argc, argv);
	}

	/* execute it */
	execvp(params->argv[0], argv);

	fprintf(stderr, "%s: execing program '%s' failed: %s\n",
			__FUNCTION__, params->argv[0], strerror(errno));

	exit(1);
}

void
loader_execute(loader_t * loader, lash_exec_params_t * params)
{
	pid_t pid;
	const char *program;

	program = params->argv[0];

	pid = fork();
	switch (pid) {
	case -1:
		fprintf(stderr,
				"%s: could not fork in order to exec program '%s': %s",
				__FUNCTION__, program, strerror(errno));
		break;

	case 0:
		loader_exec_program(loader, params);

	default:
		LASH_DEBUGARGS("forked to run program '%s'", program);
		break;
	}
}

void
loader_event(loader_t * loader, lash_comm_event_t * event)
{
	switch (lash_comm_event_get_type(event)) {
	case LASH_Comm_Event_Close:
		LASH_PRINT_DEBUG("recieved close event, exiting")
			exit(0);
	case LASH_Comm_Event_Exec:
		loader_execute(loader, lash_comm_event_take_exec(event));
		break;
	default:
		fprintf(stderr, "%s: recieved unknown event of type %d\n",
				__FUNCTION__, lash_comm_event_get_type(event));
		break;
	}
}

void
loader_run(loader_t * loader)
{
	lash_comm_event_t *event;
	int err;

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* don't need the server socket */
	err = close(loader->server_socket);
	if (err == -1) {
		fprintf(stderr, "%s: could not close server socket: %s\n",
				__FUNCTION__, strerror(errno));
	}

	/* apps don't need the loader socket */
	err = fcntl(loader->loader_socket, F_SETFD, FD_CLOEXEC);
	if (err == -1) {
		fprintf(stderr,
				"%s: could not set close-on-exec on loader socket: %s\n",
				__FUNCTION__, strerror(errno));
	}

	/* we don't care if apps finish */
	signal(SIGCHLD, SIG_IGN);

	for (;;) {
		LASH_PRINT_DEBUG("listening for event");
		event = lash_comm_event_new();
		err = lash_comm_recv_event(loader->loader_socket, event);

		if (err == -2) {
			fprintf(stderr, "%s: server closed socket; exiting\n",
					__FUNCTION__);
			exit(1);
		}

		if (err == -1) {
			fprintf(stderr, "%s: error recieving event; exiting\n",
					__FUNCTION__);
			exit(1);
		}

		LASH_PRINT_DEBUG("got event");

		loader_event(loader, event);

		lash_comm_event_destroy(event);
	}
}

int
loader_fork(loader_t * loader)
{
	pid_t pid;
	int err;

	pid = fork();

	switch (pid) {
	case -1:
		fprintf(stderr, "%s: error while forking: %s\n", __FUNCTION__,
				strerror(errno));
		return 1;

	case 0:
		loader_run(loader);
		exit(0);
		break;

	default:
		LASH_DEBUGARGS("loader running with pid %d", pid);
		loader->loader_pid = pid;
		err = close(loader->loader_socket);
		if (err == -1) {
			fprintf(stderr, "%s: error closing loader socket: %s\n",
					__FUNCTION__, strerror(errno));
		}
		break;
	}

	return 0;
}

void
loader_load(loader_t * loader, const lash_exec_params_t * params)
{
	lash_comm_event_t event;
	int err;

	lash_comm_event_set_exec(&event, (lash_exec_params_t *) params);

	LASH_PRINT_DEBUG("sending exec params");
	err = lash_comm_send_event(loader->server_socket, &event);
	if (err < 0) {
		fprintf(stderr, "%s: error sending event to the loader\n",
				__FUNCTION__);
	}
	LASH_PRINT_DEBUG("exec params sent");
}

/* EOF */
