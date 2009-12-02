/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of code that interfaces
 * app supervisor object through D-Bus
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

#include "app_supervisor_proxy.h"
#include "../dbus/helpers.h"
#include "../dbus_constants.h"

struct ladish_app_supervisor_proxy
{
  char * service;
  char * object;
  uint64_t version;
};

bool ladish_app_supervisor_proxy_create(const char * service, const char * object, ladish_app_supervisor_proxy_handle * handle_ptr)
{
  struct ladish_app_supervisor_proxy * proxy_ptr;

  proxy_ptr = malloc(sizeof(struct ladish_app_supervisor_proxy));
  if (proxy_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct proxy");
    goto fail;
  }

  proxy_ptr->service = strdup(service);
  if (proxy_ptr->service == NULL)
  {
    log_error("strdup() failed too duplicate service name '%s'", service);
    goto free_proxy;
  }

  proxy_ptr->object = strdup(object);
  if (proxy_ptr->object == NULL)
  {
    log_error("strdup() failed too duplicate object name '%s'", object);
    goto free_service;
  }

  proxy_ptr->version = 0;

  *handle_ptr = (ladish_app_supervisor_proxy_handle)proxy_ptr;

  return true;

free_service:
  free(proxy_ptr->service);

free_proxy:
  free(proxy_ptr);

fail:
  return false;
}

#define proxy_ptr ((struct ladish_app_supervisor_proxy *)proxy)

void ladish_app_supervisor_proxy_destroy(ladish_app_supervisor_proxy_handle proxy)
{
  free(proxy_ptr->object);
  free(proxy_ptr->service);
  free(proxy_ptr);
}

bool ladish_app_supervisor_proxy_run_custom(ladish_app_supervisor_proxy_handle proxy, const char * command, const char * name, bool run_in_terminal)
{
  dbus_bool_t terminal;
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "RunCustom", "bss", &terminal, &command, &name, ""))
  {
    log_error("RunCustom() failed.");
    return false;
  }

  return true;
}

#undef proxy_ptr
