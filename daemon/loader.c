/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 * Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 **************************************************************************
 * This file contains code that starts programs
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

#include <unistd.h>
#include <fcntl.h>
#include <pty.h>                /* forkpty() */
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "loader.h"
#include "../proxies/conf_proxy.h"
#include "conf.h"
#include "../common/catdup.h"

#define XTERM_COMMAND_EXTENSION "&& sh || sh"

#define CLIENT_OUTPUT_BUFFER_SIZE 2048

struct loader_child
{
  struct list_head  siblings;

  char * vgraph_name;
  char * app_name;
  char * project_name;

  bool dead;
  int exit_status;
  pid_t pid;

  bool terminal;

  int stdout;
  char stdout_buffer[CLIENT_OUTPUT_BUFFER_SIZE];
  char stdout_last_line[CLIENT_OUTPUT_BUFFER_SIZE];
  unsigned int stdout_last_line_repeat_count;
  char * stdout_buffer_ptr;

  int stderr;
  char stderr_buffer[CLIENT_OUTPUT_BUFFER_SIZE];
  char stderr_last_line[CLIENT_OUTPUT_BUFFER_SIZE];
  unsigned int stderr_last_line_repeat_count;
  char * stderr_buffer_ptr;
};

static void (* g_on_child_exit)(pid_t pid, int exit_status);
static struct list_head g_childs_list;

static struct loader_child *
loader_child_find(pid_t pid)
{
  struct list_head *node_ptr;
  struct loader_child *child_ptr;

  list_for_each (node_ptr, &g_childs_list)
  {
    child_ptr = list_entry(node_ptr, struct loader_child, siblings);
    if (child_ptr->pid == pid)
    {
      return child_ptr;
    }
  }

  return NULL;
}

static
void
loader_check_line_repeat_end(
  char * vgraph_name,
  char * app_name,
  bool error,
  unsigned int last_line_repeat_count)
{
  if (last_line_repeat_count >= 2)
  {
    if (error)
    {
      log_error_plain("%s:%s: stderr line repeated %u times", vgraph_name, app_name, last_line_repeat_count);
    }
    else
    {
      log_info("%s:%s: stdout line repeated %u times", vgraph_name, app_name, last_line_repeat_count);
    }
  }
}

static void
loader_childs_bury(void)
{
  struct list_head *node_ptr;
  struct list_head *next_ptr;
  struct loader_child *child_ptr;

  list_for_each_safe (node_ptr, next_ptr, &g_childs_list)
  {
    child_ptr = list_entry(node_ptr, struct loader_child, siblings);
    if (child_ptr->dead)
    {
      loader_check_line_repeat_end(
        child_ptr->vgraph_name,
        child_ptr->app_name,
        false,
        child_ptr->stdout_last_line_repeat_count);

      loader_check_line_repeat_end(
        child_ptr->vgraph_name,
        child_ptr->app_name,
        true,
        child_ptr->stderr_last_line_repeat_count);

      log_debug("Bury child '%s' with PID %llu", child_ptr->app_name, (unsigned long long)child_ptr->pid);

      list_del(&child_ptr->siblings);

      free(child_ptr->project_name);
      free(child_ptr->vgraph_name);
      free(child_ptr->app_name);

      if (!child_ptr->terminal)
      {
        close(child_ptr->stdout);
        close(child_ptr->stderr);
      }

      g_on_child_exit(child_ptr->pid, child_ptr->exit_status);
      free(child_ptr);
    }
  }
}

static void loader_sigchld_handler(int signum)
{
  int status;
  pid_t pid;
  struct loader_child *child_ptr;
  int signal;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    child_ptr = loader_child_find(pid);

    if (!child_ptr)
    {
      log_error("Termination of unknown child process with PID %llu detected", (unsigned long long)pid);
    }
    else
    {
      log_info("Termination of child process '%s' with PID %llu detected", child_ptr->app_name, (unsigned long long)pid);
      child_ptr->dead = true;
      child_ptr->exit_status = status;
    }

    if (WIFEXITED(status))
    {
      log_info("Child exited, code=%d", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
      signal = WTERMSIG(status);
      switch (signal)
      {
      case SIGILL:
      case SIGABRT:
      case SIGSEGV:
      case SIGFPE:
        log_error("Child was killed by signal %d", signal);
        break;
      default:
        log_info("Child was killed by signal %d", signal);
      }
    }
    else if (WIFSTOPPED(status))
    {
      log_info("Child was stopped by signal %d", WSTOPSIG(status));
    }
  }
}

void loader_init(void (* on_child_exit)(pid_t pid, int exit_status))
{
  g_on_child_exit = on_child_exit;
  signal(SIGCHLD, loader_sigchld_handler);
  INIT_LIST_HEAD(&g_childs_list);
}

void loader_uninit(void)
{
  loader_childs_bury();
}

#if 0
static void loader_exec_program_in_xterm(const char * const * argv)
{
  char * dst_ptr;
  const char * const * src_ptr_ptr;
  size_t len;

  log_debug("Executing program '%s' with PID %llu in terminal", argv[0], (unsigned long long)getpid());

  /* Calculate the command string length */
  len = strlen(XTERM_COMMAND_EXTENSION) + 1;
  for (src_ptr_ptr = argv; *src_ptr_ptr != NULL; src_ptr_ptr++)
  {
    len += strlen(*src_ptr_ptr) + 3; /* three additional chars per argument: two double quotes and a space */
  }

  char buf[len];                /* dynamically allocate in stack */

  /* Create the command string */
  src_ptr_ptr = argv;
  dst_ptr = buf;
  while (*src_ptr_ptr != NULL)
  {
    len = strlen(*src_ptr_ptr);
    dst_ptr[0] = '"';
    memcpy(dst_ptr + 1, src_ptr_ptr, len);
    dst_ptr[1 + len] = '"';
    dst_ptr[1 + len + 1] = ' ';
    dst_ptr += len + 3;
    src_ptr_ptr++;
  }

  strcat(dst_ptr, XTERM_COMMAND_EXTENSION);

  /* Execute the command */
  execlp("xterm", "xterm", "-e", "/bin/sh", "-c", buf, NULL);

  log_error("Failed to execute command '%s' in terminal: %s", buf, strerror(errno));

  exit(1);
}
#endif

static
void
loader_exec_program(
  const char * commandline,
  const char * working_dir,
  const char * session_dir,
  bool run_in_terminal,
  const char * vgraph_name,
  const char * project_name,
  const char * app_name)
{
  const char * argv[8];
  unsigned int i;

  /* for non terminal processes we use forkpty() that calls login_tty() that calls setsid() */
  /* we can successful call setsid() only once */
  if (run_in_terminal)
  {
    /* no longer anything to do with lashd */
    if (setsid() == -1)
    {
      fprintf(stderr, "Could not create new process group: %s\n", strerror(errno));
    }
  }

  /* change the working dir */
  if (chdir(working_dir) == -1)
  {
    fprintf(stderr, "Could not change directory to working dir '%s' for app '%s': %s\n", working_dir, app_name, strerror(errno));
  }

  setenv("LADISH_APP_NAME", app_name, true);
  setenv("LADISH_VGRAPH_NAME", vgraph_name, true);

  if (project_name != NULL)
  {
    setenv("LADISH_PROJECT_NAME", project_name, true);
  }

  if (session_dir != NULL)
  {
    setenv("SESSION_DIR", session_dir, true);
  }

  i = 0;

  if (run_in_terminal)
  {
    if (!conf_get(LADISH_CONF_KEY_DAEMON_TERMINAL, argv + i))
    {
      argv[i] = LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT;
    }
    i++;

    if (strcmp(argv[0], "xterm") == 0 &&
        strchr(app_name, '"') == NULL &&
        strchr(app_name, '\'') == NULL &&
        strchr(app_name, '`') == NULL)
    {
      argv[i++] = "-T";
      argv[i++] = app_name;
    }

    argv[i++] = "-e";
  }

  if (!conf_get(LADISH_CONF_KEY_DAEMON_SHELL, argv + i))
  {
    argv[i] = LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT;
  }
  i++;

  argv[i++] = "-c";

  argv[i++] = commandline;
  argv[i++] = NULL;

  printf("Executing '%s' with PID %llu\n", commandline, (unsigned long long)getpid());

  /* Execute it */
  execvp(argv[0], (char **)argv);

  fprintf(stderr, "Executing program '%s' failed: %s\n", argv[0], strerror(errno));

  exit(1);
}

static
void
loader_read_child_output(
  char * vgraph_name,
  char * app_name,
  int fd,
  bool error,
  char * buffer_ptr,
  char ** buffer_ptr_ptr,
  char * last_line,
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
              log_error_plain("%s:%s: last stderr line repeating..", vgraph_name, app_name);
            }
            else
            {
              log_info("%s:%s: last stdout line repeating...", vgraph_name, app_name);
            }
          }

          (*last_line_repeat_count)++;
        }
        else
        {
          loader_check_line_repeat_end(vgraph_name, app_name, error, *last_line_repeat_count);

          strcpy(last_line, char_ptr);
          *last_line_repeat_count = 1;

          if (error)
          {
            log_error_plain("%s:%s: %s", vgraph_name, app_name, char_ptr);
          }
          else
          {
            log_info("%s:%s: %s", vgraph_name, app_name, char_ptr);
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
            log_error_plain("%s:%s: %s " ANSI_RESET ANSI_COLOR_RED "(truncated) " ANSI_RESET, vgraph_name, app_name, char_ptr);
          }
          else
          {
            log_info("%s:%s: %s " ANSI_RESET ANSI_COLOR_RED "(truncated) " ANSI_RESET, vgraph_name, app_name, char_ptr);
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
  while (ret == max_read);      /* if we have read everything as much as we can, then maybe there is more to read */
}

static void
loader_read_childs_output(void)
{
  struct list_head * node_ptr;
  struct loader_child * child_ptr;

  list_for_each (node_ptr, &g_childs_list)
  {
    child_ptr = list_entry(node_ptr, struct loader_child, siblings);

    if (!child_ptr->terminal)
    {
      loader_read_child_output(
        child_ptr->vgraph_name,
        child_ptr->app_name,
        child_ptr->stdout,
        false,
        child_ptr->stdout_buffer,
        &child_ptr->stdout_buffer_ptr,
        child_ptr->stdout_last_line,
        &child_ptr->stdout_last_line_repeat_count);

      loader_read_child_output(
        child_ptr->vgraph_name,
        child_ptr->app_name,
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

#define LD_PRELOAD_ADD "libalsapid.so libasound.so.2"

static void set_ldpreload(void)
{
  const char * old;
  char * new;

  old = getenv("LD_PRELOAD");
  if (old != NULL)
  {
    new = catdup3(LD_PRELOAD_ADD, " ", old);
    if (new == NULL)
    {
      fprintf(stderr, "Memory allocation failure. Cannot hook libalsapid.so through LD_PRELOAD.\n");
      return;
    }
  }
  else
  {
    new = LD_PRELOAD_ADD;
  }

  printf("LD_PRELOAD set to \"%s\"\n", new);
  setenv("LD_PRELOAD", new, 1);

  if (old)
  {
    free(new);
  }
}

bool
loader_execute(
  const char * vgraph_name,
  const char * project_name,
  const char * app_name,
  const char * working_dir,
  const char * session_dir,
  bool run_in_terminal,
  const char * commandline,
  pid_t * pid_ptr)
{
  pid_t pid;
  struct loader_child * child_ptr;
  int stderr_pipe[2];

  child_ptr = malloc(sizeof(struct loader_child));
  if (child_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct loader_child");
    goto fail;
  }

  child_ptr->vgraph_name = strdup(vgraph_name);
  if (child_ptr->vgraph_name == NULL)
  {
    log_error("strdup() failed to duplicate vgraph name '%s'", vgraph_name);
    goto free_struct;
  }

  if (project_name != NULL)
  {
    child_ptr->project_name = strdup(app_name);
    if (child_ptr->project_name == NULL)
    {
      log_error("strdup() failed to duplicate project name '%s'", project_name);
      goto free_vgraph_name;
    }
  }
  else
  {
    child_ptr->project_name = NULL;
  }

  child_ptr->app_name = strdup(app_name);
  if (child_ptr->app_name == NULL)
  {
    log_error("strdup() failed to duplicate app name '%s'", app_name);
    goto free_project_name;
  }

  child_ptr->dead = false;
  child_ptr->terminal = run_in_terminal;
  child_ptr->stdout_buffer_ptr = child_ptr->stdout_buffer;
  child_ptr->stderr_buffer_ptr = child_ptr->stderr_buffer;
  child_ptr->stdout_last_line_repeat_count = 0;
  child_ptr->stderr_last_line_repeat_count = 0;

  if (!run_in_terminal)
  {
    if (pipe(stderr_pipe) == -1)
    {
      log_error("Failed to create stderr pipe");
    }
    else
    {
      child_ptr->stderr = stderr_pipe[0];

      if (fcntl(child_ptr->stderr, F_SETFL, O_NONBLOCK) == -1)
      {
        log_error("Failed to set nonblocking mode on "
                   "stderr reading end: %s",
                   strerror(errno));
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
      }
    }
  }

  list_add_tail(&child_ptr->siblings, &g_childs_list);

  if (!run_in_terminal)
  {
    /* We need pty to disable libc buffering of stdout */
    pid = forkpty(&child_ptr->stdout, NULL, NULL, NULL);
  }
  else
  {
    pid = fork();
  }

  if (pid == -1)
  {
    log_error("Could not fork to exec program %s:%s: %s", vgraph_name, app_name, strerror(errno));
    list_del(&child_ptr->siblings); /* fork failed so it is not really a child process to watch for. */
    return false;
  }

  if (pid == 0)
  {
    /* Need to close all open file descriptors except the std ones */
    struct rlimit max_fds;
    rlim_t fd;

    getrlimit(RLIMIT_NOFILE, &max_fds);

    for (fd = 3; fd < max_fds.rlim_cur; ++fd)
    {
      close(fd);
    }

    if (!run_in_terminal)
    {
      /* In child, close unused reading end of pipe */
      close(stderr_pipe[0]);

      dup2(stderr_pipe[1], fileno(stderr));
    }

    set_ldpreload();

    loader_exec_program(commandline, working_dir, session_dir, run_in_terminal, vgraph_name, project_name, app_name);

    return false;  /* We should never get here */
  }

  if (!run_in_terminal)
  {
    /* In parent, close unused writing ends of pipe */
    close(stderr_pipe[1]);

    if (fcntl(child_ptr->stdout, F_SETFL, O_NONBLOCK) == -1)
    {
      log_error("Could not set noblocking mode on stdout "
                 "- pty: %s", strerror(errno));
      close(stderr_pipe[0]);
      close(child_ptr->stdout);
    }
  }

  log_info("Forked to run program %s:%s pid = %llu", vgraph_name, app_name, (unsigned long long)pid);

  *pid_ptr = child_ptr->pid = pid;

  return true;

free_project_name:
  free(child_ptr->project_name);

free_vgraph_name:
  free(child_ptr->vgraph_name);

free_struct:
  free(child_ptr);

fail:
  return false;
}

unsigned int loader_get_app_count(void)
{
  struct list_head * node_ptr;
  unsigned int count;

  count = 0;

  list_for_each(node_ptr, &g_childs_list)
  {
    count++;
  }

  return count;
}
