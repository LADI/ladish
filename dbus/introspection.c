/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains the D-Bus introspection interface handler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "../common/safety.h"
#include "../common/debug.h"
#include "introspection.h"
#include "signal.h"
#include "method.h"

#define write_buf(args...) buf_ptr += sprintf(buf_ptr, ## args)

DBusMessage *
introspection_new(struct dbus_object_path * opath_ptr)
{
  char *xml_data, *buf_ptr;
  const struct dbus_object_path_interface * iface_ptr;
  const method_t * method_ptr;
  const method_arg_t * method_arg_ptr;
  const signal_t * signal_ptr;
  const signal_arg_t * signal_arg_ptr;
  DBusMessage * msg;
  DBusMessageIter iter;

  lash_debug("Creating introspection message");

  /*
   * Create introspection XML data.
   */

  xml_data = lash_malloc(1, 16384);
  buf_ptr = xml_data;

  write_buf("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
            " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node name=\"%s\">\n", opath_ptr->name);

  /* Add the object path's interfaces. */
  for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
  {
    write_buf("  <interface name=\"%s\">\n", iface_ptr->iface->name);
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
    write_buf("  </interface>\n");
  }
  write_buf("</node>");

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
      lash_error("Failed to append data to introspection message");
    }
  }
  else
  {
    lash_error("Failed to create introspection message");
  }

  free(xml_data);
  return msg;
}

#undef write_buf

void
introspection_destroy(struct dbus_object_path *path)
{
  lash_debug("Destroying introspection message");

  if (path && path->introspection) {
    dbus_message_unref(path->introspection);
    path->introspection = NULL;
  }
#ifdef LASH_DEBUG
  else
    lash_debug("Nothing to destroy");
#endif
}

static bool introspection_handler(const interface_t * interface, method_call_t * call_ptr)
{
  if (strcmp(call_ptr->method_name, "Introspect") != 0)
  {
    /* The requested method wasn't "Introspect". */
    return false;
  }

  /* Try to construct the instrospection message */
  call_ptr->reply = dbus_message_copy(call_ptr->context); /* context contains the reply message */
  if (call_ptr->reply == NULL)
  {
    lash_error("Ran out of memory trying to copy introspection message");
    goto fail;
  }

  if (!dbus_message_set_destination(call_ptr->reply, dbus_message_get_sender(call_ptr->message)))
  {
    lash_error("dbus_message_set_destination() failed.");
    goto unref_reply;
  }

  if (!dbus_message_set_reply_serial(call_ptr->reply, dbus_message_get_serial(call_ptr->message)))
  {
    lash_error("dbus_message_set_reply_serial() failed.");
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

METHOD_ARGS_BEGIN(Introspect, "Get introspection XML")
  METHOD_ARG_DESCRIBE_OUT("xml_data", "s", "XML description of the object")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Introspect, NULL)
METHODS_END

INTERFACE_BEGIN(g_dbus_interface_dtor_introspectable, "org.freedesktop.DBus.Introspectable")
  INTERFACE_HANDLER(introspection_handler)
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
