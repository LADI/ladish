/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains constants for D-Bus service and interface names and for D-Bus object paths
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

#ifndef DBUS_CONSTANTS_H__C21DE0EE_C19C_42F0_8D63_D613E4806C0E__INCLUDED
#define DBUS_CONSTANTS_H__C21DE0EE_C19C_42F0_8D63_D613E4806C0E__INCLUDED

#define JACKDBUS_SERVICE_NAME    "org.jackaudio.service"
#define JACKDBUS_OBJECT_PATH     "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"
#define JACKDBUS_IFACE_CONFIGURE "org.jackaudio.Configure"
#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"

#define SERVICE_NAME             DBUS_NAME_BASE
#define CONTROL_OBJECT_PATH      DBUS_BASE_PATH "/Control"
#define IFACE_CONTROL            DBUS_NAME_BASE ".Control"
#define STUDIO_OBJECT_PATH       DBUS_BASE_PATH "/Studio"
#define IFACE_STUDIO             DBUS_NAME_BASE ".Studio"
#define IFACE_ROOM               DBUS_NAME_BASE ".Room"
#define IFACE_APP_SUPERVISOR     DBUS_NAME_BASE ".AppSupervisor"
#define APPLICATION_OBJECT_PATH  DBUS_BASE_PATH "/Application"
#define IFACE_APPLICATION        DBUS_NAME_BASE ".App"
#define IFACE_GRAPH_DICT         DBUS_NAME_BASE ".GraphDict"
#define IFACE_GRAPH_MANAGER      DBUS_NAME_BASE ".GraphManager"
#define IFACE_RECENT_ITEMS       DBUS_NAME_BASE ".RecentItems"
#define LASH_SERVER_OBJECT_PATH  DBUS_BASE_PATH "/LashServer"
#define IFACE_LASH_SERVER        DBUS_NAME_BASE ".LashServer"
#define IFACE_LASH_CLIENT        DBUS_NAME_BASE ".LashClient"

#define JMCORE_SERVICE_NAME      DBUS_NAME_BASE ".jmcore"
#define JMCORE_IFACE             JMCORE_SERVICE_NAME
#define JMCORE_OBJECT_PATH       DBUS_BASE_PATH "/jmcore"

#define CONF_SERVICE_NAME        DBUS_NAME_BASE ".conf"
#define CONF_IFACE               CONF_SERVICE_NAME
#define CONF_OBJECT_PATH         DBUS_BASE_PATH "/conf"

#define JACKDBUS_PORT_FLAG_INPUT         0x00000001
#define JACKDBUS_PORT_FLAG_OUTPUT        0x00000002
#define JACKDBUS_PORT_FLAG_PHYSICAL      0x00000004
#define JACKDBUS_PORT_FLAG_CAN_MONITOR   0x00000008
#define JACKDBUS_PORT_FLAG_TERMINAL      0x00000010

#define JACKDBUS_PORT_TYPE_AUDIO    0
#define JACKDBUS_PORT_TYPE_MIDI     1

#define GRAPH_DICT_OBJECT_TYPE_GRAPH          0
#define GRAPH_DICT_OBJECT_TYPE_CLIENT         1
#define GRAPH_DICT_OBJECT_TYPE_PORT           2
#define GRAPH_DICT_OBJECT_TYPE_CONNECTION     3

#define URI_CANVAS_WIDTH    "http://ladish.org/ns/canvas/width"
#define URI_CANVAS_HEIGHT   "http://ladish.org/ns/canvas/height"
#define URI_CANVAS_X        "http://ladish.org/ns/canvas/x"
#define URI_CANVAS_Y        "http://ladish.org/ns/canvas/y"
#define URI_A2J_PORT        "http://ladish.org/ns/a2j"

#define JACKDBUS_PORT_IS_INPUT(flags) (((flags) & JACKDBUS_PORT_FLAG_INPUT) != 0)
#define JACKDBUS_PORT_IS_OUTPUT(flags) (((flags) & JACKDBUS_PORT_FLAG_OUTPUT) != 0)

#define JACKDBUS_PORT_SET_INPUT(flags) (flags) |= JACKDBUS_PORT_FLAG_INPUT
#define JACKDBUS_PORT_SET_OUTPUT(flags) (flags) |= JACKDBUS_PORT_FLAG_OUTPUT

#define JACKDBUS_PORT_CLEAR_INPUT(flags) (flags) &= ~JACKDBUS_PORT_FLAG_INPUT
#define JACKDBUS_PORT_CLEAR_OUTPUT(flags) (flags) &= ~JACKDBUS_PORT_FLAG_OUTPUT

#endif /* #ifndef DBUS_CONSTANTS_H__C21DE0EE_C19C_42F0_8D63_D613E4806C0E__INCLUDED */
