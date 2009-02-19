/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <uuid/uuid.h>

#include "common/safety.h"
#include "common/debug.h"
#include "common/klist.h"

#include "lash/types.h"

#include "loader.h"
#include "server.h"
#include "client.h"
#include "project.h"
#include "sigsegv.h"

#define XTERM_COMMAND_EXTENSION "&& sh || sh"

#define CLIENT_OUTPUT_BUFFER_SIZE 2048

struct loader_child
{
	struct list_head  siblings;

	uuid_t            id;
	char             *project;
	char             *argv0;

	bool              dead;
	pid_t             pid;

	bool              terminal;
	int               stdout;
	char              stdout_buffer[CLIENT_OUTPUT_BUFFER_SIZE];
	char              stdout_last_line[CLIENT_OUTPUT_BUFFER_SIZE];
	unsigned int      stdout_last_line_repeat_count;
	char             *stdout_buffer_ptr;
	int               stderr;
	char              stderr_buffer[CLIENT_OUTPUT_BUFFER_SIZE];
	char              stderr_last_line[CLIENT_OUTPUT_BUFFER_SIZE];
	unsigned int      stderr_last_line_repeat_count;
	char             *stderr_buffer_ptr;
};

static struct list_head g_childs_list;

static struct loader_child *
loader_child_find_and_mark_dead(pid_t pid)
{
	struct list_head *node_ptr;
	struct loader_child *child_ptr;

	list_for_each (node_ptr, &g_childs_list) {
		child_ptr = list_entry(node_ptr, struct loader_child, siblings);
		if (child_ptr->pid == pid) {
			child_ptr->dead = true;
			return child_ptr;
		}
	}

	return NULL;
}

static
void
loader_check_line_repeat_end(
	char * project,
	char * argv0,
	bool error,
	unsigned int last_line_repeat_count)
{
	if (last_line_repeat_count >= 2)
	{
		if (error)
		{
			lash_error_plain("%s:%s: stderr line repeated %u times", project, argv0, last_line_repeat_count);
		}
		else
		{
			lash_info("%s:%s: stdout line repeated %u times", project, argv0, last_line_repeat_count);
		}
	}
}

static void
loader_childs_bury(void)
{
	struct list_head *node_ptr;
	struct list_head *next_ptr;
	struct loader_child *child_ptr;

	list_for_each_safe (node_ptr, next_ptr, &g_childs_list) {
		child_ptr = list_entry(node_ptr, struct loader_child, siblings);
		if (child_ptr->dead) {
			loader_check_line_repeat_end(
				child_ptr->project,
				child_ptr->argv0,
				false,
				child_ptr->stdout_last_line_repeat_count);
			loader_check_line_repeat_end(
				child_ptr->project,
				child_ptr->argv0,
				true,
				child_ptr->stderr_last_line_repeat_count);

			lash_debug("Bury child '%s' with PID %u",
			           child_ptr->argv0, (unsigned int) child_ptr->pid);

			list_del(&child_ptr->siblings);

			lash_free(&child_ptr->project);
			lash_free(&child_ptr->argv0);

			if (!child_ptr->terminal) {
				close(child_ptr->stdout);
				close(child_ptr->stderr);
			}

			client_disconnected(server_find_client_by_pid(child_ptr->pid));
			free(child_ptr);
		}
	}
}

static void
loader_sigchld_handler(int signum)
{
	int status;
	pid_t pid;
	struct loader_child *child_ptr;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		child_ptr = loader_child_find_and_mark_dead(pid);

		if (!child_ptr)
			lash_error("LASH loader detected termination of "
			           "unknown child process with PID %u",
			           (unsigned int) pid);
		else
			lash_info("LASH loader detected termination of "
			          "child process '%s' with PID %u",
			          child_ptr->argv0, (unsigned int) pid);

		if (WIFEXITED(status))
			lash_info("Child exited, status=%d", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			lash_info("Child was killed by signal %d", WTERMSIG(status));
		else if (WIFSTOPPED(status))
			lash_info("Child was stopped by signal %d", WSTOPSIG(status));
	}
}

void
loader_init(void)
{
	signal(SIGCHLD, loader_sigchld_handler);
	INIT_LIST_HEAD(&g_childs_list);
}

void
loader_uninit(void)
{
	loader_childs_bury();
}

static void
loader_exec_program_in_xterm(char **argv)
{
	char *ptr, **aptr;
	size_t len;

	lash_debug("Executing program '%s' with PID %u in terminal",
	           argv[0], (unsigned int) getpid());

	/* Calculate the command string length */
	len = strlen(XTERM_COMMAND_EXTENSION) + 1;
	for (aptr = argv; *aptr; ++aptr)
		len += strlen(*aptr) + 3;

	char buf[len];
	ptr = (char *) buf;

	/* Create the command string */
	for (aptr = argv; *aptr; ++aptr) {
		sprintf(ptr, "\"%s\" ", *aptr);
		ptr += strlen(*aptr) + 3;
	}
	sprintf(ptr, "%s", XTERM_COMMAND_EXTENSION);

	/* Execute the command */
	execlp("xterm", "xterm", "-e", "/bin/sh", "-c", buf, (char *) NULL);

	lash_error("Failed to execute command '%s' in terminal: %s",
	           buf, strerror(errno));

	exit(1);
}

static void
loader_exec_program(client_t *client,
                    bool      run_in_terminal)
{
	/* for non terminal processes we use forkpty() that calls login_tty() that calls setsid() */
	/* we can successful call setsid() only once */
	if (run_in_terminal) {
		/* no longer anything to do with lashd */
		if (setsid() == -1) {
			lash_error("Could not create new process group: %s",
			           strerror(errno));
		}
	}

	/* change the working dir */
	if (chdir(client->working_dir) == -1) {
		lash_error("Could not change directory to working "
		           "dir '%s' for program '%s': %s",
		           client->working_dir, client->argv[0],
		           strerror(errno));
	}

#ifdef LASH_DEBUG
	char *ptr, **aptr;
	size_t len = 0;

	for (aptr = client->argv; *aptr; ++aptr)
		len += strlen(*aptr) + 1;

	char buf[len];
	ptr = (char *) buf;

	for (aptr = client->argv; *aptr; ++aptr) {
		strcpy(ptr, *aptr);
		ptr += strlen(*aptr);
		*ptr = ' ';
		++ptr;
	}
	*ptr = '\0';

	lash_debug("Running command: %s", buf);
#endif

	if (run_in_terminal)
		loader_exec_program_in_xterm(client->argv);

	lash_info("Executing program '%s' with PID %u",
	          client->argv[0], (unsigned int) getpid());

	/* Execute it */
	execvp(client->argv[0], client->argv);

	lash_error("Executing program '%s' failed: %s",
	           client->argv[0], strerror(errno));

	exit(1);
}

static void
loader_read_child_output(
	char  *project,
	char  *argv0,
	int    fd,
	bool   error,
	char  *buffer_ptr,
	char **buffer_ptr_ptr,
	char  *last_line,
	unsigned int * last_line_repeat_count)
{
	ssize_t ret;
	char *char_ptr;
	char *eol_ptr;
	size_t left;
	size_t max_read;

	do
	{
		max_read = CLIENT_OUTPUT_BUFFER_SIZE - 1 - (*buffer_ptr_ptr - buffer_ptr);
		ret = read(fd, *buffer_ptr_ptr, max_read);
		if (ret > 0)
		{
			(*buffer_ptr_ptr)[ret] = 0;
			char_ptr = buffer_ptr;

			while ((eol_ptr = strchr(char_ptr, '\n')) != NULL)
			{
				*eol_ptr = 0;

				if (*last_line_repeat_count > 0 && strcmp(last_line, char_ptr) == 0)
				{
					if (*last_line_repeat_count == 1)
					{
						if (error)
						{
							lash_error_plain("%s:%s: last stderr line repeating..", project, argv0);
						}
						else
						{
							lash_info("%s:%s: last stdout line repeating...", project, argv0);
						}
					}

					(*last_line_repeat_count)++;
				}
				else
				{
					loader_check_line_repeat_end(project, argv0, error, *last_line_repeat_count);

					strcpy(last_line, char_ptr);
					*last_line_repeat_count = 1;

					if (error)
					{
						lash_error_plain("%s:%s: %s", project, argv0, char_ptr);
					}
					else
					{
						lash_info("%s:%s: %s", project, argv0, char_ptr);
					}
				}

				char_ptr = eol_ptr + 1;
			}

			left = ret - (char_ptr - *buffer_ptr_ptr);
			if (left != 0)
			{
				/* last line does not end with newline */

				if (left == CLIENT_OUTPUT_BUFFER_SIZE - 1)
				{
					/* line is too long to fit in buffer */
					/* print it like it is, rest (or more) of it will be logged on next interation */

					if (error)
					{
						lash_error_plain("%s:%s: %s " ANSI_RESET ANSI_COLOR_RED "(truncated) " ANSI_RESET, project, argv0, char_ptr);
					}
					else
					{
						lash_info("%s:%s: %s " ANSI_RESET ANSI_COLOR_RED "(truncated) " ANSI_RESET, project, argv0, char_ptr);
					}

					left = 0;
				}
				else
				{
					memmove(buffer_ptr, char_ptr, left);
				}
			}

			*buffer_ptr_ptr = buffer_ptr + left;
		}
	}
	while (ret == max_read);			/* if we have read everything as much as we can, then maybe there is more to read */
}

static void
loader_read_childs_output(void)
{
	struct list_head *node_ptr;
	struct loader_child *child_ptr;

	list_for_each (node_ptr, &g_childs_list) {
		child_ptr = list_entry(node_ptr, struct loader_child, siblings);

		if (!child_ptr->dead && !child_ptr->terminal)
		{
			loader_read_child_output(
				child_ptr->project,
				child_ptr->argv0,
				child_ptr->stdout,
				false,
				child_ptr->stdout_buffer,
				&child_ptr->stdout_buffer_ptr,
				child_ptr->stdout_last_line,
				&child_ptr->stdout_last_line_repeat_count);

			loader_read_child_output(
				child_ptr->project,
				child_ptr->argv0,
				child_ptr->stderr,
				true,
				child_ptr->stderr_buffer,
				&child_ptr->stderr_buffer_ptr,
				child_ptr->stderr_last_line,
				&child_ptr->stderr_last_line_repeat_count);
		}
	}
}

void
loader_run(void)
{
	loader_read_childs_output();
	loader_childs_bury();
}

void
loader_execute(client_t *client,
               bool      run_in_terminal)
{
	pid_t pid;
	const char *program;
	struct loader_child *child_ptr;
	int stderr_pipe[2];

	program = client->argv[0];

	child_ptr = lash_calloc(1, sizeof(struct loader_child));

	uuid_copy(child_ptr->id, client->id);
	child_ptr->project = lash_strdup(client->project->name);
	child_ptr->argv0 = lash_strdup(program);
	child_ptr->terminal = run_in_terminal;
	child_ptr->stdout_buffer_ptr = child_ptr->stdout_buffer;
	child_ptr->stderr_buffer_ptr = child_ptr->stderr_buffer;
	child_ptr->stdout_last_line_repeat_count = 0;
	child_ptr->stderr_last_line_repeat_count = 0;

	if (!child_ptr->terminal) {
		if (pipe(stderr_pipe) == -1) {
			lash_error("Failed to create stderr pipe");
		} else {
			child_ptr->stderr = stderr_pipe[0];

			if (fcntl(child_ptr->stderr, F_SETFL, O_NONBLOCK) == -1) {
				lash_error("Failed to set nonblocking mode on "
				           "stderr reading end: %s",
				           strerror(errno));
				close(stderr_pipe[0]);
				close(stderr_pipe[1]);
			}
		}
	}

	list_add_tail(&child_ptr->siblings, &g_childs_list);

	if (!child_ptr->terminal)
		/* We need pty to disable libc buffering of stdout */
		pid = forkpty(&child_ptr->stdout, NULL, NULL, NULL);
	else
		pid = fork();

	if (pid == -1) {
		lash_error("Could not fork to exec program '%s': %s",
		           program, strerror(errno));
		client->pid = 0;
		return;
	}

	if (pid == 0) {
		char pid_str[21];
		/* Need to close all open file descriptors except the std ones */
		struct rlimit max_fds;
		rlim_t fd;

		getrlimit(RLIMIT_NOFILE, &max_fds);

		for (fd = 3; fd < max_fds.rlim_cur; ++fd)
			close(fd);

		if (!child_ptr->terminal) {
			/* In child, close unused reading end of pipe */
			close(stderr_pipe[0]);

			dup2(stderr_pipe[1], fileno(stderr));
		}

		/* Let the client know it is being restored */
		sprintf(pid_str, "%d", getpid());
		setenv("LASH_CLIENT_IS_BEING_RESTORED", pid_str, 1);

		loader_exec_program(client, run_in_terminal);

		return;  /* We should never get here */
	}

	if (!child_ptr->terminal) {
		/* In parent, close unused writing ends of pipe */
		close(stderr_pipe[1]);

		if (fcntl(child_ptr->stdout, F_SETFL, O_NONBLOCK) == -1) {
			lash_error("Could not set noblocking mode on stdout "
			           "- pty: %s", strerror(errno));
			close(stderr_pipe[0]);
			close(child_ptr->stdout);
		}
	}

	client->pid = pid;
	child_ptr->pid = pid;
	lash_debug("Forked to run program '%s'", program);
}
