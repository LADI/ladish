/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to app supervisor object
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

#ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED
#define APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED

#include "common.h"

typedef struct ladish_app_supervisor_tag { int unused; } * ladish_app_supervisor_handle;

bool
ladish_app_supervisor_create(
  ladish_app_supervisor_handle * supervisor_handle_ptr,
  const char * opath,
  const char * name,
  void * context,
  void (* on_app_renamed)(void * context, const char * old_name, const char * new_app_name));

void
ladish_app_supervisor_destroy(
  ladish_app_supervisor_handle supervisor_handle);

bool
ladish_app_supervisor_child_exit(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

bool
ladish_app_supervisor_enum(
  ladish_app_supervisor_handle supervisor_handle,
  void * context,
  bool (* callback)(void * context, const char * name, bool running, const char * command, bool terminal, uint8_t level, pid_t pid));

void
ladish_app_supervisor_clear(
  ladish_app_supervisor_handle supervisor_handle);

bool
ladish_app_supervisor_add(
  ladish_app_supervisor_handle supervisor_handle,
  const char * name,
  bool autorun,
  const char * command,
  bool terminal,
  uint8_t level);

void
ladish_app_supervisor_stop(
  ladish_app_supervisor_handle supervisor_handle);

void
ladish_app_supervisor_autorun(
  ladish_app_supervisor_handle supervisor_handle);

char *
ladish_app_supervisor_search_app(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

const char * ladish_app_supervisor_get_name(ladish_app_supervisor_handle supervisor_handle);

extern const struct dbus_interface_descriptor g_iface_app_supervisor;

#endif /* #ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED */
