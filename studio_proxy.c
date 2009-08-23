/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the helper functionality
 * for accessing Studio object through D-Bus
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
#include "dbus_constants.h"
#include "dbus/helpers.h"

bool studio_proxy_get_name(char ** name_ptr)
{
  const char * name;
  if (!dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "GetName", "", "s", &name))
  {
    return false;
  }

  *name_ptr = strdup(name);
  if (*name_ptr == NULL)
  {
    lash_error("strdup() failed to duplicate studio name");
    return false;
  }

  return true;
}

bool studio_proxy_rename(const char * name)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Rename", "s", &name, "");
}

bool studio_proxy_save(void)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Save", "", "");
}
