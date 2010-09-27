/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains defines for conf keys
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

#ifndef CONF_H__795797BE_4EB8_44F8_BD9C_B8A9CB975228__INCLUDED
#define CONF_H__795797BE_4EB8_44F8_BD9C_B8A9CB975228__INCLUDED

#define LADISH_CONF_KEY_DAEMON_NOTIFY             "/org/ladish/daemon/notify"
#define LADISH_CONF_KEY_DAEMON_SHELL              "/org/ladish/daemon/shell"
#define LADISH_CONF_KEY_DAEMON_TERMINAL           "/org/ladish/daemon/terminal"
#define LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART   "/org/ladish/daemon/studio_autostart"

#define LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT             true
#define LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT              "sh"
#define LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT           "xterm"
#define LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT   true

#endif /* #ifndef CONF_H__795797BE_4EB8_44F8_BD9C_B8A9CB975228__INCLUDED */
