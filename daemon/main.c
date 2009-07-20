/*
 *   LASH
 *
 *   Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "common.h"

#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#include "version.h"            /* git version define */
#include "proctitle.h"
#include "loader.h"
#include "sigsegv.h"
#include "dbus_iface_control.h"

bool g_quit;
service_t * g_dbus_service;

#if 0
static DBusHandlerResult lashd_client_disconnect_handler(DBusConnection * connection, DBusMessage * message, void * data)
{
  /* If the message isn't a signal the object path handler may use it */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  const char *member, *name, *old_name;
  struct lash_client *client;
  DBusError err;

  if (!(member = dbus_message_get_member(message)))
  {
    lash_error("Received JACK signal with NULL member");
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(member, "NameOwnerChanged") != 0)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  dbus_error_init(&err);

  if (!dbus_message_get_args(message, &err,
                             DBUS_TYPE_STRING, &name,
                             DBUS_TYPE_STRING, &old_name,
                             DBUS_TYPE_INVALID))
  {
    lash_error("Cannot get message arguments: %s",
               err.message);
    dbus_error_free(&err);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  client = server_find_client_by_dbus_name(old_name);
  if (client)
  {
    client_disconnected(client);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  else
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}
#endif

service_t * lashd_dbus_service_new(void)
{
  service_t * service;
  DBusError err;

  service = service_new(
    DBUS_NAME_BASE,
    &g_quit,
    1,
    object_path_new(
      "/",
      NULL,
      1,//2,
      //&g_lashd_interface_server,
      &g_lashd_interface_control,
      NULL),
    NULL);
  if (service == NULL)
  {
    lash_error("Failed to create D-Bus service");
    return NULL;
  }

  dbus_error_init(&err);

#if 0
  dbus_bus_add_match(
    service->connection,
    "type='signal'"
    ",sender='org.freedesktop.DBus'"
    ",path='/org/freedesktop/DBus'"
    ",interface='org.freedesktop.DBus'"
    ",member='NameOwnerChanged'"
    ",arg2=''",
    &err);
  if (dbus_error_is_set(&err))
  {
    lash_error("Failed to add D-Bus match rule: %s", err.message);
    dbus_error_free(&err);
    goto fail;
  }

  if (!dbus_connection_add_filter(service->connection, lashd_client_disconnect_handler, NULL, NULL))
  {
    lash_error("Failed to add D-Bus filter");
    goto fail;
  }
#endif

  return service;

#if 0
fail:
  service_destroy(service);
  return NULL;
#endif
}

static void on_child_exit(pid_t pid)
{
  //client_disconnected(server_find_client_by_pid(child_ptr->pid));
}

void term_signal_handler(int signum)
{
  lash_info("Caught signal %d (%s), terminating", signum, strsignal(signum));
  g_quit = true;
}

bool install_term_signal_handler(int signum, bool ignore_if_already_ignored)
{
  sig_t sigh;

  sigh = signal(signum, term_signal_handler);
  if (sigh == SIG_ERR)
  {
    lash_error("signal() failed to install handler function for signal %d.", signum);
    return false;
  }

  if (sigh == SIG_IGN && ignore_if_already_ignored)
  {
    signal(SIGTERM, SIG_IGN);
  }

  return true;
}

int main(int argc, char ** argv, char ** envp)
{
  struct stat st;
  char timestamp_str[26];
  int ret;

  st.st_mtime = 0;
  stat(argv[0], &st);
  ctime_r(&st.st_mtime, timestamp_str);
  timestamp_str[24] = 0;

  lash_init_setproctitle(argc, argv, envp);

  dbus_threads_init_default();

  lash_info("------------------");
  lash_info("LADI session handler activated. Version %s (%s) built on %s", PACKAGE_VERSION, GIT_VERSION, timestamp_str);

  ret = EXIT_FAILURE;

  loader_init(on_child_exit);

  g_dbus_service = lashd_dbus_service_new();
  if (g_dbus_service == NULL)
  {
    lash_error("Failed to launch D-Bus service");
    goto uninit_loader;
  }

  /* install the signal handlers */
  install_term_signal_handler(SIGTERM, false);
  install_term_signal_handler(SIGINT, true);
  install_term_signal_handler(SIGHUP, true);
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    lash_error("signal(SIGPIPE, SIG_IGN).");
  }

  /* setup our SIGSEGV magic that prints nice stack in our logfile */ 
  setup_sigsegv();

  while (!g_quit)
  {
    dbus_connection_read_write_dispatch(g_dbus_service->connection, 50);
    loader_run();
  }

  ret = EXIT_SUCCESS;

  lash_debug("Finished, cleaning up");

  service_destroy(g_dbus_service);

uninit_loader:
  loader_uninit();

  lash_debug("Cleaned up, exiting");

  lash_info("LADI session handler deactivated");
  lash_info("------------------");

  exit(EXIT_SUCCESS);
}
