/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * The D-Bus patchbay manager
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

#include "graph_manager.h"
#include "../dbus/error.h"
#include "../dbus_constants.h"
#include "graph.h"
#include "virtualizer.h"

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

#define graph ((ladish_graph_handle)call_ptr->iface_context)

static void ladish_graph_manager_dbus_split(struct dbus_method_call * call_ptr)
{
  uint64_t client_id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("split request, graph '%s', client %"PRIu64, ladish_graph_get_description(graph), client_id);

  if (!ladish_virtualizer_split_client(graph, client_id))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "split failed");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_graph_manager_dbus_join(struct dbus_method_call * call_ptr)
{
  uint64_t client1_id;
  uint64_t client2_id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &client1_id,
        DBUS_TYPE_UINT64, &client2_id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("join request, graph '%s', client1 %"PRIu64", client2 %"PRIu64, ladish_graph_get_description(graph), client1_id, client2_id);

  if (!ladish_virtualizer_join_clients(graph, client1_id, client2_id))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "join failed");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_graph_manager_dbus_rename_client(struct dbus_method_call * call_ptr)
{
  uint64_t client_id;
  const char * newname;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &newname,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("rename client request, graph '%s', client %"PRIu64", newname '%s'", ladish_graph_get_description(graph), client_id, newname);

  method_return_new_void(call_ptr);
}

static void ladish_graph_manager_dbus_rename_port(struct dbus_method_call * call_ptr)
{
  uint64_t port_id;
  const char * newname;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_STRING, &newname,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("rename port request, graph '%s', port %"PRIu64", newname '%s'", ladish_graph_get_description(graph), port_id, newname);

  method_return_new_void(call_ptr);
}

static void ladish_graph_manager_dbus_move_port(struct dbus_method_call * call_ptr)
{
  uint64_t port_id;
  uint64_t client_id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("move port request, graph '%s', port %"PRIu64", client %"PRIu64, ladish_graph_get_description(graph), port_id, client_id);

  method_return_new_void(call_ptr);
}

static void ladish_graph_manager_dbus_new_client(struct dbus_method_call * call_ptr)
{
  const char * name;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("new client request, graph '%s', name '%s'", ladish_graph_get_description(graph), name);

  lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "method \"%s\" not implemented yet",  call_ptr->method_name);
}

#undef graph_ptr

METHOD_ARGS_BEGIN(Split, "Split client")
  METHOD_ARG_DESCRIBE_IN("client_id", "t", "ID of the client")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Join, "Join two clients")
  METHOD_ARG_DESCRIBE_IN("client1_id", "t", "ID of the first client")
  METHOD_ARG_DESCRIBE_IN("client2_id", "t", "ID of the second client")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(RenameClient, "Rename client")
  METHOD_ARG_DESCRIBE_IN("client_id", "t", "ID of the client")
  METHOD_ARG_DESCRIBE_IN("newname", "s", "New name for the client")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(RenamePort, "Rename port")
  METHOD_ARG_DESCRIBE_IN("port_id", "t", "ID of the port")
  METHOD_ARG_DESCRIBE_IN("newname", "s", "New name for the port")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(MovePort, "Move port")
  METHOD_ARG_DESCRIBE_IN("port_id", "t", "ID of the port")
  METHOD_ARG_DESCRIBE_IN("client_id", "t", "ID of the client where port will be moved to")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(NewClient, "New client")
  METHOD_ARG_DESCRIBE_IN("name", "s", "Name for the new client")
  METHOD_ARG_DESCRIBE_OUT("client_id", "t", "ID of the new client")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Split, ladish_graph_manager_dbus_split)
  METHOD_DESCRIBE(Join, ladish_graph_manager_dbus_join)
  METHOD_DESCRIBE(RenameClient, ladish_graph_manager_dbus_rename_client)
  METHOD_DESCRIBE(RenamePort, ladish_graph_manager_dbus_rename_port)
  METHOD_DESCRIBE(MovePort, ladish_graph_manager_dbus_move_port)
  METHOD_DESCRIBE(NewClient, ladish_graph_manager_dbus_new_client)
METHODS_END

INTERFACE_BEGIN(g_iface_graph_manager, IFACE_GRAPH_MANAGER)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END

