/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 * Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 **************************************************************************
 * This file contains the code that implements main() and other top-level functionality
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

#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#include "version.h"            /* git version define */
#include "proctitle.h"
#include "loader.h"
#include "sigsegv.h"
#include "control.h"
#include "studio.h"
#include "../dbus_constants.h"
#include "../common/catdup.h"
#include "../common/dirhelpers.h"
#include "../proxies/a2j_proxy.h"
#include "../proxies/jmcore_proxy.h"
#include "../proxies/notify_proxy.h"
#include "../proxies/conf_proxy.h"
#include "conf.h"

bool g_quit;
const char * g_dbus_unique_name;
dbus_object_path g_control_object;
char * g_base_dir;
static bool g_use_notify = false;

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

  if (!(member = dbus_message_get_member(message)))
  {
    log_error("Received JACK signal with NULL member");
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
    log_error("Cannot get message arguments: %s",
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
    log_error("Failed to add D-Bus match rule: %s", err.message);
    dbus_error_free(&err);
    goto fail;
  }

  if (!dbus_connection_add_filter(service->connection, lashd_client_disconnect_handler, NULL, NULL))
  {
    log_error("Failed to add D-Bus filter");
    goto fail;
  }
#endif

static bool connect_dbus(void)
{
  int ret;

  dbus_error_init(&g_dbus_error);

  g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &g_dbus_error);
  if (dbus_error_is_set(&g_dbus_error))
  {
    log_error("Failed to get bus: %s", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    goto fail;
  }

  g_dbus_unique_name = dbus_bus_get_unique_name(g_dbus_connection);
  if (g_dbus_unique_name == NULL)
  {
    log_error("Failed to read unique bus name");
    goto unref_connection;
  }

  log_info("Connected to local session bus, unique name is \"%s\"", g_dbus_unique_name);

  ret = dbus_bus_request_name(g_dbus_connection, SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &g_dbus_error);
  if (ret == -1)
  {
    log_error("Failed to acquire bus name: %s", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    goto unref_connection;
  }

  if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
  {
    log_error("Requested connection name already exists");
    goto unref_connection;
  }

  g_control_object = dbus_object_path_new(CONTROL_OBJECT_PATH, &g_lashd_interface_control, NULL, NULL);
  if (g_control_object == NULL)
  {
    goto unref_connection;
  }

  if (!dbus_object_path_register(g_dbus_connection, g_control_object))
  {
    goto destroy_control_object;
  }

  return true;

destroy_control_object:
  dbus_object_path_destroy(g_dbus_connection, g_control_object);
unref_connection:
  dbus_connection_unref(g_dbus_connection);

fail:
  return false;
}

static void disconnect_dbus(void)
{
  dbus_object_path_destroy(g_dbus_connection, g_control_object);
  dbus_connection_unref(g_dbus_connection);
}

void term_signal_handler(int signum)
{
  log_info("Caught signal %d (%s), terminating", signum, strsignal(signum));
  g_quit = true;
}

bool install_term_signal_handler(int signum, bool ignore_if_already_ignored)
{
  sig_t sigh;

  sigh = signal(signum, term_signal_handler);
  if (sigh == SIG_ERR)
  {
    log_error("signal() failed to install handler function for signal %d.", signum);
    return false;
  }

  if (sigh == SIG_IGN && ignore_if_already_ignored)
  {
    signal(SIGTERM, SIG_IGN);
  }

  return true;
}

bool init_paths(void)
{
  const char * home_dir;

  home_dir = getenv("HOME");
  if (home_dir == NULL)
  {
    log_error("Environment variable HOME not set");
    return false;
  }

  g_base_dir = catdup(home_dir, BASE_DIR);
  if (g_base_dir == NULL)
  {
    log_error("catdup failed for '%s' and '%s'", home_dir, BASE_DIR);
    return false;
  }

  if (!ensure_dir_exist(g_base_dir, 0700))
  {
    free(g_base_dir);
    return false;
  }

  return true;
}

void uninit_paths(void)
{
  free(g_base_dir);
}

static void on_conf_notify_changed(void * context, const char * key, const char * value)
{
  bool notify_enable;

  if (value == NULL)
  {
    notify_enable = LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT;
  }
  else
  {
    notify_enable = conf_string2bool(value);
  }

  if (notify_enable)
  {
    if (!g_use_notify)
    {
      g_use_notify = ladish_notify_init("LADI Session Handler");
    }
  }
  else
  {
    log_info("Sending notifications is disabled");
    if (g_use_notify)
    {
      ladish_notify_uninit();
      g_use_notify = false;
    }
  }
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

  log_info("------------------");
  log_info("LADI session handler activated. Version %s (%s) built on %s", PACKAGE_VERSION, GIT_VERSION, timestamp_str);

  ret = EXIT_FAILURE;

  if (!init_paths())
  {
    goto exit;
  }

  loader_init(ladish_studio_on_child_exit);

  if (!room_templates_init())
  {
    goto uninit_loader;
  }

  if (!connect_dbus())
  {
    log_error("Failed to connecto to D-Bus");
    goto uninit_room_templates;
  }

  /* install the signal handlers */
  install_term_signal_handler(SIGTERM, false);
  install_term_signal_handler(SIGINT, true);
  install_term_signal_handler(SIGHUP, true);
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    log_error("signal(SIGPIPE, SIG_IGN).");
  }

  /* setup our SIGSEGV magic that prints nice stack in our logfile */ 
  setup_sigsegv();

  if (!conf_proxy_init())
  {
    goto uninit_dbus;
  }

  if (!conf_register(LADISH_CONF_KEY_DAEMON_NOTIFY, on_conf_notify_changed, NULL))
  {
    goto uninit_conf;
  }

  if (!conf_register(LADISH_CONF_KEY_DAEMON_SHELL, NULL, NULL))
  {
    goto uninit_conf;
  }

  if (!conf_register(LADISH_CONF_KEY_DAEMON_TERMINAL, NULL, NULL))
  {
    goto uninit_conf;
  }

  if (!conf_register(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, NULL, NULL))
  {
    goto uninit_conf;
  }

  if (!a2j_proxy_init())
  {
    goto uninit_conf;
  }

  if (!jmcore_proxy_init())
  {
    goto uninit_a2j;
  }

  if (!ladish_studio_init())
  {
    goto uninit_jmcore;
  }

  ladish_notify_simple(LADISH_NOTIFY_URGENCY_LOW, "LADI Session Handler daemon activated", NULL);

  while (!g_quit)
  {
    dbus_connection_read_write_dispatch(g_dbus_connection, 50);
    loader_run();
    ladish_studio_run();
  }

  emit_clean_exit();
  ladish_notify_simple(LADISH_NOTIFY_URGENCY_LOW, "LADI Session Handler daemon deactivated", NULL);

  ret = EXIT_SUCCESS;

  log_debug("Finished, cleaning up");

  ladish_studio_uninit();

uninit_jmcore:
  jmcore_proxy_uninit();

uninit_a2j:
  a2j_proxy_uninit();

uninit_conf:
  if (g_use_notify)
  {
    ladish_notify_uninit();
  }

  conf_proxy_uninit();

uninit_dbus:
  disconnect_dbus();

uninit_room_templates:
  room_templates_uninit();

uninit_loader:
  loader_uninit();

  uninit_paths();

exit:
  log_debug("Cleaned up, exiting");

  log_info("LADI session handler deactivated");
  log_info("------------------");

  exit(EXIT_SUCCESS);
}
