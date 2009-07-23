/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Helper functionality for accessing JACK through D-Bus
 *
 *****************************************************************************/

//#define LASH_DEBUG

#include "jack_proxy.h"

#define JACKDBUS_SERVICE         "org.jackaudio.service"
#define JACKDBUS_OBJECT          "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"
#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"
#define JACKDBUS_IFACE_CONFIGURE "org.jackaudio.Configure"

static
void
on_jack_patchbay_signal(
  DBusMessage * message_ptr,
  const char * signal_name)
{
  const char * client1_name;
  const char * port1_name;
  const char * client2_name;
  const char * port2_name;
  dbus_uint64_t dummy;
  dbus_uint64_t client1_id;
  dbus_uint64_t client2_id;
  dbus_uint64_t port1_id;
  dbus_uint64_t port2_id;

  dbus_error_init(&g_dbus_error);

  if (strcmp(signal_name, "ClientAppeared") == 0)
  {
    lash_debug("Received ClientAppeared signal");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &dummy,
          DBUS_TYPE_UINT64, &client1_id,
          DBUS_TYPE_STRING, &client1_name,
          DBUS_TYPE_INVALID))
    {
      goto fail;
    }

    on_jack_client_appeared(client1_id, client1_name);
    return;
  }

  if (strcmp(signal_name, "ClientDisappeared") == 0)
  {
    lash_debug("Received ClientDisappeared signal");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &dummy,
          DBUS_TYPE_UINT64, &client1_id,
          DBUS_TYPE_STRING, &client1_name,
          DBUS_TYPE_INVALID))
    {
      goto fail;
    }

    on_jack_client_disappeared(client1_id);
    return;
  }

  if (strcmp(signal_name, "PortAppeared") == 0)
  {
    lash_debug("Received PortAppeared signal");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &dummy,
          DBUS_TYPE_UINT64, &client1_id,
          DBUS_TYPE_STRING, &client1_name,
          DBUS_TYPE_UINT64, &port1_id,
          DBUS_TYPE_STRING, &port1_name,
          DBUS_TYPE_INVALID))
    {
      goto fail;
    }

    on_jack_port_appeared(client1_id, port1_id, port1_name);
    return;
  }

  if (strcmp(signal_name, "PortsConnected") == 0)
  {
    lash_debug("Received PortsConnected signal");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &dummy,
          DBUS_TYPE_UINT64, &client1_id,
          DBUS_TYPE_STRING, &client1_name,
          DBUS_TYPE_UINT64, &port1_id,
          DBUS_TYPE_STRING, &port1_name,
          DBUS_TYPE_UINT64, &client2_id,
          DBUS_TYPE_STRING, &client2_name,
          DBUS_TYPE_UINT64, &port2_id,
          DBUS_TYPE_STRING, &port2_name,
          DBUS_TYPE_INVALID))
    {
      goto fail;
    }

    on_jack_ports_connected(
      client1_id,
      port1_id,
      client2_id,
      port2_id);

    return;
  }

  if (strcmp(signal_name, "PortsDisconnected") == 0)
  {
    lash_debug("Received PortsDisconnected signal");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &dummy,
          DBUS_TYPE_UINT64, &client1_id,
          DBUS_TYPE_STRING, &client1_name,
          DBUS_TYPE_UINT64, &port1_id,
          DBUS_TYPE_STRING, &port1_name,
          DBUS_TYPE_UINT64, &client2_id,
          DBUS_TYPE_STRING, &client2_name,
          DBUS_TYPE_UINT64, &port2_id,
          DBUS_TYPE_STRING, &port2_name,
          DBUS_TYPE_INVALID))
    {
      goto fail;
    }

    on_jack_ports_disconnected(
      client1_id,
      port1_id,
      client2_id,
      port2_id);
    return;
  }

  return;

fail:
  lash_error("Cannot get message arguments: %s", g_dbus_error.message);
  dbus_error_free(&g_dbus_error);
}

static
void
on_jack_control_signal(
  DBusMessage * message_ptr,
  const char * signal_name)
{
  if (strcmp(signal_name, "ServerStarted") == 0)
  {
    lash_debug("JACK server start detected.");
    on_jack_server_started();
    return;
  }

  if (strcmp(signal_name, "ServerStopped") == 0)
  {
    lash_debug("JACK server stop detected.");
    on_jack_server_stopped();
    return;
  }
}

static
DBusHandlerResult
on_bus_signal(
  DBusMessage * message_ptr,
  const char * signal_name)
{
  const char * object_name;
  const char * old_owner;
  const char * new_owner;

  //lash_info("bus signal '%s' received", signal_name);

  dbus_error_init(&g_dbus_error);

  if (strcmp(signal_name, "NameOwnerChanged") == 0)
  {
    //lash_info("NameOwnerChanged signal received");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_STRING, &object_name,
          DBUS_TYPE_STRING, &old_owner,
          DBUS_TYPE_STRING, &new_owner,
          DBUS_TYPE_INVALID)) {
      lash_error("Cannot get message arguments: %s", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(object_name, JACKDBUS_SERVICE) != 0)
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (old_owner[0] == '\0')
    {
      lash_debug("JACK serivce appeared");
      on_jack_server_appeared();
    }
    else if (new_owner[0] == '\0')
    {
      lash_debug("JACK serivce disappeared");
      on_jack_server_disappeared();
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

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

  lash_debug("'%s' sent signal '%s'::'%s'", object_path, interface, signal_name);

  /* Handle JACK patchbay and control interface signals */
  if (object_path != NULL && strcmp(object_path, JACKDBUS_OBJECT) == 0)
  {
    if (strcmp(interface, JACKDBUS_IFACE_PATCHBAY) == 0)
    {
      on_jack_patchbay_signal(message_ptr, signal_name);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(interface, JACKDBUS_IFACE_CONTROL) == 0)
    {
      on_jack_control_signal(message_ptr, signal_name);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  /* Handle session bus signals to track JACK service alive state */
  if (strcmp(interface, DBUS_INTERFACE_DBUS) == 0)
  {
    return on_bus_signal(message_ptr, signal_name);
  }

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

bool
jack_proxy_read_conf_container(
  const char * address,
  char * child_buffer,
  size_t child_buffer_size,
  void * callback_context,
  bool (* callback)(void * context, bool leaf, const char * address, char * child))
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  DBusMessageIter top_iter;
  DBusMessageIter array_iter;
  const char * component;
  const char * reply_signature;
  dbus_bool_t leaf;           /* Whether children are parameters (true) or containers (false) */
  char * child;

  request_ptr = dbus_message_new_method_call(JACKDBUS_SERVICE, JACKDBUS_OBJECT, JACKDBUS_IFACE_CONFIGURE, "ReadContainer");
  if (request_ptr == NULL)
  {
    lash_error("dbus_message_new_method_call() failed.");
    return false;
  }

  dbus_message_iter_init_append(request_ptr, &top_iter);

  if (!dbus_message_iter_open_container(&top_iter, DBUS_TYPE_ARRAY, "s", &array_iter))
  {
    lash_error("dbus_message_iter_open_container() failed.");
    dbus_message_unref(request_ptr);
    return false;
  }

  if (address != NULL)
  {
    component = address;
    while (*component != 0)
    {
      if (!dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &component))
      {
        lash_error("dbus_message_iter_append_basic() failed.");
        dbus_message_unref(request_ptr);
        return false;
      }

      component += strlen(component) + 1;
    }
  }

  dbus_message_iter_close_container(&top_iter, &array_iter);

  // send message and get a handle for a reply
  reply_ptr = dbus_connection_send_with_reply_and_block(
    g_dbus_connection,
    request_ptr,
    DBUS_CALL_DEFAULT_TIMEOUT,
    &g_dbus_error);

  dbus_message_unref(request_ptr);

  if (reply_ptr == NULL)
  {
    lash_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "bas") != 0)
  {
    lash_error("ReadContainer() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);

  dbus_message_iter_get_basic(&top_iter, &leaf);
  dbus_message_iter_next(&top_iter);

  dbus_message_iter_recurse(&top_iter, &array_iter);

  while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
  {
    dbus_message_iter_get_basic(&array_iter, &child);

    if (!callback(callback_context, leaf, address, child))
    {
      break;
    }

    dbus_message_iter_next(&array_iter);
  }

  dbus_message_unref(reply_ptr);

  return true;
}
