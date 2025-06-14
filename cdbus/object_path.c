/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus object path helpers
 **************************************************************************
 *
 * Licensed under the Academic Free License version 2.1
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

#include "../common.h"
#include "helpers.h"

struct cdbus_object_path_interface
{
  const struct cdbus_interface_descriptor * iface;
  void * iface_context;
};

struct cdbus_object_path
{
  char * name;
  DBusMessage * introspection;
  struct cdbus_object_path_interface * ifaces;
  bool registered;
};

#define write_buf(args...) buf_ptr += sprintf(buf_ptr, ## args)

DBusMessage * cdbus_introspection_new(struct cdbus_object_path * opath_ptr)
{
  char *xml_data, *buf_ptr;
  const struct cdbus_object_path_interface * iface_ptr;
  const struct cdbus_method_descriptor * method_ptr;
  const struct cdbus_method_arg_descriptor * method_arg_ptr;
  const struct cdbus_signal_descriptor * signal_ptr;
  const struct cdbus_signal_arg_descriptor * signal_arg_ptr;
  DBusMessage * msg;
  DBusMessageIter iter;

  log_debug("Creating introspection message");

  /*
   * Create introspection XML data.
   */

  /* TODO: we assume that 16 KiB is enough to hold introspection xml.
   * If it gets larger, memory corruption will occur.
   * Use realloc-like algorithm instead */
  xml_data = malloc(16384);
  if (xml_data == NULL)
  {
    return NULL;
  }

  buf_ptr = xml_data;

  write_buf("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
            " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node name=\"%s\">\n", opath_ptr->name);

  /* Add the object path's interfaces. */
  for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
  {
    write_buf("  <interface name=\"%s\">\n", iface_ptr->iface->name);
    if (iface_ptr->iface->methods != NULL)
    {
      /* Add the interface's methods. */
      for (method_ptr = iface_ptr->iface->methods; method_ptr->name != NULL; method_ptr++)
      {
        write_buf("    <method name=\"%s\">\n", method_ptr->name);
        /* Add the method's arguments. */
        for (method_arg_ptr = method_ptr->args; method_arg_ptr->name != NULL; method_arg_ptr++)
        {
          write_buf(
            "      <arg name=\"%s\" type=\"%s\" direction=\"%s\" />\n",
            method_arg_ptr->name,
            method_arg_ptr->type,
            method_arg_ptr->direction_in ? "in" : "out");
        }
        write_buf("    </method>\n");
      }
    }
    if (iface_ptr->iface->signals != NULL)
    {
      /* Add the interface's signals. */
      for (signal_ptr = iface_ptr->iface->signals; signal_ptr->name != NULL; signal_ptr++)
      {
        write_buf("    <signal name=\"%s\">\n", signal_ptr->name);
        /* Add the signal's arguments. */
        for (signal_arg_ptr = signal_ptr->args; signal_arg_ptr->name != NULL; signal_arg_ptr++)
        {
          write_buf("      <arg name=\"%s\" type=\"%s\" />\n", signal_arg_ptr->name, signal_arg_ptr->type);
        }
        write_buf("    </signal>\n");
      }
    }
    write_buf("  </interface>\n");
  }
  write_buf("</node>\n");

  /*
   * Create a D-Bus message from the XML data.
   */

  if ((msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_RETURN)))
  {
    dbus_message_iter_init_append(msg, &iter);
    if (dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, (const void *) &xml_data))
    {
      dbus_message_set_no_reply(msg, TRUE);
    }
    else
    {
      dbus_message_unref(msg);
      msg = NULL;
      log_error("Failed to append data to introspection message");
    }
  }
  else
  {
    log_error("Failed to create introspection message");
  }

  free(xml_data);
  return msg;
}

#undef write_buf

void cdbus_introspection_destroy(struct cdbus_object_path *path)
{
  log_debug("Destroying introspection message");

  if (path && path->introspection) {
    dbus_message_unref(path->introspection);
    path->introspection = NULL;
  }
  else
  {
    log_debug("Nothing to destroy");
  }
}

static
bool
cdbus_introspection_handler(
  const struct cdbus_interface_descriptor * UNUSED(interface),
  struct cdbus_method_call * call_ptr)
{
  if (strcmp(call_ptr->method_name, "Introspect") != 0)
  {
    /* The requested method wasn't "Introspect". */
    return false;
  }

  /* Try to construct the instrospection message */
  call_ptr->reply = dbus_message_copy(call_ptr->iface_context); /* context contains the reply message */
  if (call_ptr->reply == NULL)
  {
    log_error("Ran out of memory trying to copy introspection message");
    goto fail;
  }

  if (!dbus_message_set_destination(call_ptr->reply, dbus_message_get_sender(call_ptr->message)))
  {
    log_error("dbus_message_set_destination() failed.");
    goto unref_reply;
  }

  if (!dbus_message_set_reply_serial(call_ptr->reply, dbus_message_get_serial(call_ptr->message)))
  {
    log_error("dbus_message_set_reply_serial() failed.");
    goto unref_reply;
  }

  return true;

unref_reply:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  /* Even after an error we need to return true, because the
     handler is only supposed to return false if a nonexistent
     method is requested. */
  return true;
}

CDBUS_METHOD_ARGS_BEGIN(Introspect, "Get introspection XML")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("xml_data", "s", "XML description of the object")
CDBUS_METHOD_ARGS_END

CDBUS_METHODS_BEGIN
  CDBUS_METHOD_DESCRIBE(Introspect, NULL)
CDBUS_METHODS_END

CDBUS_INTERFACE_BEGIN(g_dbus_interface_dtor_introspectable, "org.freedesktop.DBus.Introspectable")
  CDBUS_INTERFACE_HANDLER(cdbus_introspection_handler)
  CDBUS_INTERFACE_EXPOSE_METHODS
CDBUS_INTERFACE_END

cdbus_object_path cdbus_object_path_new(const char *name, const struct cdbus_interface_descriptor * iface1_ptr, ...)
{
  struct cdbus_object_path * opath_ptr;
  va_list ap;
  const struct cdbus_interface_descriptor * iface_src_ptr;
  struct cdbus_object_path_interface * iface_dst_ptr;
  size_t len;

  log_debug("Creating object path");

  opath_ptr = malloc(sizeof(struct cdbus_object_path));
  if (opath_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct cdbus_object_path.");
    goto fail;
  }
  
  opath_ptr->name = strdup(name);
  if (opath_ptr->name == NULL)
  {
    log_error("malloc() failed to allocate struct cdbus_object_path.");
    goto free;
  }

  va_start(ap, iface1_ptr);
  iface_src_ptr = iface1_ptr;
  len = 0;
  while (iface_src_ptr != NULL)
  {
    va_arg(ap, void *);         /* skip interface context */
    iface_src_ptr = va_arg(ap, const struct cdbus_interface_descriptor *);
    len++;
  }
  va_end(ap);

  opath_ptr->ifaces = malloc((len + 2) * sizeof(struct cdbus_object_path_interface));
  if (opath_ptr->ifaces == NULL)
  {
    log_error("malloc failed to allocate interfaces array");
    goto free_name;
  }

  va_start(ap, iface1_ptr);
  iface_src_ptr = iface1_ptr;
  iface_dst_ptr = opath_ptr->ifaces;
  while (iface_src_ptr != NULL)
  {
    iface_dst_ptr->iface = iface_src_ptr;
    iface_dst_ptr->iface_context = va_arg(ap, void *);
    iface_src_ptr = va_arg(ap, const struct cdbus_interface_descriptor *);
    iface_dst_ptr++;
    len--;
  }
  va_end(ap);

  ASSERT(len == 0);

  iface_dst_ptr->iface = NULL;
  opath_ptr->introspection = cdbus_introspection_new(opath_ptr);
  if (opath_ptr->introspection == NULL)
  {
    log_error("introspection_new() failed.");
    goto free_ifaces;
  }

  iface_dst_ptr->iface = &g_dbus_interface_dtor_introspectable;
  iface_dst_ptr->iface_context = opath_ptr->introspection;
  iface_dst_ptr++;
  iface_dst_ptr->iface = NULL;

  opath_ptr->registered = false;

  return (cdbus_object_path)opath_ptr;

free_ifaces:
  free(opath_ptr->ifaces);
free_name:
  free(opath_ptr->name);
free:
  free(opath_ptr);
fail:
  return NULL;
}

#define opath_ptr ((struct cdbus_object_path *)data)

void cdbus_object_path_unregister(DBusConnection * connection_ptr, cdbus_object_path data)
{
  ASSERT(opath_ptr->registered);

  if (!dbus_connection_unregister_object_path(connection_ptr, opath_ptr->name))
  {
    log_error("dbus_connection_unregister_object_path() failed.");
  }
}

void cdbus_object_path_destroy(DBusConnection * connection_ptr, cdbus_object_path data)
{
  log_debug("Destroying object path");

  if (opath_ptr->registered && connection_ptr != NULL && !dbus_connection_unregister_object_path(connection_ptr, opath_ptr->name))
  {
    log_error("dbus_connection_unregister_object_path() failed.");
  }

  cdbus_introspection_destroy(opath_ptr);
  free(opath_ptr->ifaces);
  free(opath_ptr->name);
  free(opath_ptr);
}

static DBusHandlerResult cdbus_object_path_handler(DBusConnection * connection, DBusMessage * message, void * data)
{
  const char * iface_name;
  const struct cdbus_object_path_interface * iface_ptr;
  struct cdbus_method_call call = {};

  /* Check if the message is a method call. If not, ignore it. */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
  {
    goto handled;
  }

  /* Get the invoked method's name and make sure it's non-NULL. */
  call.method_name = dbus_message_get_member(message);
  if (call.method_name == NULL)
  {
    cdbus_error(&call, DBUS_ERROR_UNKNOWN_METHOD, "Received method call with empty method name");
    goto send_return;
  }

  /* Initialize our data. */
  call.connection = connection;
  call.message = message;
  call.iface = NULL; /* To be set by the default interface handler */
  call.reply = NULL;

  /* Check if there's an interface specified for this method call. */
  iface_name = dbus_message_get_interface(message);
  if (iface_name != NULL)
  {
    for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
    {
      if (strcmp(iface_name, iface_ptr->iface->name) == 0)
      {
        call.iface_context = iface_ptr->iface_context;
        if (!iface_ptr->iface->handler(iface_ptr->iface, &call))
        {
          /* unknown method */
          break;
        }

        goto send_return;
      }
    }
  }
  else
  {
    /* No interface was specified so we have to try them all. D-Bus spec states:
     *
     * Optionally, the message has an INTERFACE field giving the interface the method is a part of.
     * In the absence of an INTERFACE field, if two interfaces on the same object have a method with
     * the same name, it is undefined which of the two methods will be invoked.
     * Implementations may also choose to return an error in this ambiguous case.
     * However, if a method name is unique implementations must not require an interface field.
     */
    for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
    {
      call.iface_context = iface_ptr->iface_context;
      if (!iface_ptr->iface->handler(iface_ptr->iface, &call))
      {
        /* known method */
        goto send_return;
      }
    }
  }

  cdbus_error(&call, DBUS_ERROR_UNKNOWN_METHOD, "Method \"%s\" with signature \"%s\" on interface \"%s\" doesn't exist", call.method_name, dbus_message_get_signature(message), iface_name);

send_return:
  cdbus_method_return_send(&call);

handled:
  return DBUS_HANDLER_RESULT_HANDLED;
}

static void cdbus_object_path_handler_unregister(DBusConnection * UNUSED(connection_ptr), void * data)
{
  log_debug("Message handler of object path %s was unregistered", (opath_ptr && opath_ptr->name) ? opath_ptr->name : "<unknown>");
}

bool cdbus_object_path_register(DBusConnection * connection_ptr, cdbus_object_path data)
{
  log_debug("Registering object path \"%s\"", opath_ptr->name);

  ASSERT(!opath_ptr->registered);

  DBusObjectPathVTable vtable =
  {
    cdbus_object_path_handler_unregister,
    cdbus_object_path_handler,
    NULL, NULL, NULL, NULL
  };

  if (!dbus_connection_register_object_path(connection_ptr, opath_ptr->name, &vtable, opath_ptr))
  {
    log_error("dbus_connection_register_object_path() failed.");
    return false;
  }

  opath_ptr->registered = true;
  return true;
}

#undef opath_ptr
