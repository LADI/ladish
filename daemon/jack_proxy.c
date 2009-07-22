/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Helper functionality for accessing JACK through D-Bus
 *
 *****************************************************************************/

#include "jack_proxy.h"

#define JACKDBUS_SERVICE         "org.jackaudio.service"
#define JACKDBUS_OBJECT          "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"
#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"

static
DBusHandlerResult
dbus_signal_handler(
  DBusConnection * connection_ptr,
  DBusMessage * message_ptr,
  void * data)
{
  const char * object_path;
  const char * interface;
  const char * signal_name;

  /* Non-signal messages are ignored */
  if (dbus_message_get_type(message_ptr) != DBUS_MESSAGE_TYPE_SIGNAL)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  interface = dbus_message_get_interface(message_ptr);
  if (interface == NULL)
  {
    /* Signals with no interface are ignored */
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  object_path = dbus_message_get_path(message_ptr);

  signal_name = dbus_message_get_member(message_ptr);
  if (signal_name == NULL)
  {
    lash_error("Received signal with NULL member");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  lash_info("'%s' sent signal '%s'::'%s'", object_path, interface, signal_name);

#if 0
  /* Handle JACK patchbay and control interface signals */
  if (object_path != NULL && strcmp(object_path, JACKDBUS_OBJECT) == 0)
  {
    if (strcmp(interface, JACKDBUS_IFACE_PATCHBAY) == 0)
    {
      lashd_jackdbus_handle_patchbay_signal(message_ptr);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(interface, JACKDBUS_IFACE_CONTROL) == 0)
    {
      lashd_jackdbus_handle_control_signal(message_ptr);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  /* Handle session bus signals to track JACK service alive state */
  if (strcmp(interface, DBUS_INTERFACE_DBUS) == 0)
  {
    return lashd_jackdbus_handle_bus_signal(message_ptr);
  }
#endif

  /* Let everything else pass through */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool
jack_proxy_init(
  void)
{
  DBusError err;
  char rule[1024];
  const char ** signal;

  const char * patchbay_signals[] = {
    "ClientAppeared",
    "ClientDisappeared",
    "PortAppeared",
    "PortsConnected",
    "PortsDisconnected",
    NULL};

  const char * control_signals[] = {
    "ServerStarted",
    "ServerStopped",
    NULL};

  dbus_bus_add_match(
    g_dbus_connection,
    "type='signal',interface='"DBUS_INTERFACE_DBUS"',member=NameOwnerChanged,arg0='"JACKDBUS_SERVICE"'",
    &err);
  if (dbus_error_is_set(&err))
  {
    lash_error("Failed to add D-Bus match rule: %s", err.message);
    dbus_error_free(&err);
    return false;
  }

  for (signal = patchbay_signals; *signal != NULL; signal++)
  {
    snprintf(
      rule,
      sizeof(rule),
      "type='signal',sender='"JACKDBUS_SERVICE"',path='"JACKDBUS_OBJECT"',interface='"JACKDBUS_IFACE_PATCHBAY"',member='%s'",
      *signal);

    dbus_bus_add_match(g_dbus_connection, rule, &err);
    if (dbus_error_is_set(&err))
    {
      lash_error("Failed to add D-Bus match rule: %s", err.message);
      dbus_error_free(&err);
      return false;
    }
  }

  for (signal = control_signals; *signal != NULL; signal++)
  {
    snprintf(
      rule,
      sizeof(rule),
      "type='signal',sender='"JACKDBUS_SERVICE"',path='"JACKDBUS_OBJECT"',interface='"JACKDBUS_IFACE_CONTROL"',member='%s'",
      *signal);

    dbus_bus_add_match(g_dbus_connection, rule, &err);
    if (dbus_error_is_set(&err))
    {
      lash_error("Failed to add D-Bus match rule: %s", err.message);
      dbus_error_free(&err);
      return false;
    }
  }

  if (!dbus_connection_add_filter(g_dbus_connection, dbus_signal_handler, NULL, NULL))
  {
    lash_error("Failed to add D-Bus filter");
    return false;
  }

  return true;
}

void
jack_proxy_uninit(
  void)
{
  dbus_connection_remove_filter(g_dbus_connection, dbus_signal_handler, NULL);
}

bool
jack_proxy_is_started(
  bool * started_ptr)
{
  return false;
}

bool
jack_proxy_get_client_pid(
  uint64_t client_id,
  pid_t * pid_ptr)
{
  return false;
}

bool
jack_proxy_connect_ports(
  uint64_t port1_id,
  uint64_t port2_id)
{
  return false;
}
