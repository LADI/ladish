/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to app supervisor object that is backed through D-Bus
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

#ifndef APP_SUPERVISOR_PROXY_H__A48C609D_0AB6_4C91_A9B0_BC7F1B7E4CB4__INCLUDED
#define APP_SUPERVISOR_PROXY_H__A48C609D_0AB6_4C91_A9B0_BC7F1B7E4CB4__INCLUDED

#include "common.h"

typedef struct ladish_app_supervisor_proxy_tag { int unused; } * ladish_app_supervisor_proxy_handle;

bool
ladish_app_supervisor_proxy_create(
  const char * service,
  const char * object,
  void * context,
  void (* app_added)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level),
  void (* app_state_changed)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level),
  void (* app_removed)(void * context, uint64_t id),
  ladish_app_supervisor_proxy_handle * proxy_ptr);

void ladish_app_supervisor_proxy_destroy(ladish_app_supervisor_proxy_handle proxy);

bool
ladish_app_supervisor_proxy_run_custom(
  ladish_app_supervisor_proxy_handle proxy,
  const char * command,
  const char * name,
  bool run_in_terminal,
  const char * level);

bool ladish_app_supervisor_proxy_start_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id);
bool ladish_app_supervisor_proxy_stop_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id);
bool ladish_app_supervisor_proxy_kill_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id);
bool ladish_app_supervisor_proxy_remove_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id);

bool
ladish_app_supervisor_get_app_properties(
  ladish_app_supervisor_proxy_handle proxy,
  uint64_t id,
  char ** name_ptr_ptr,
  char ** command_ptr_ptr,
  bool * running_ptr,
  bool * terminal_ptr,
  const char ** level_ptr);

bool
ladish_app_supervisor_set_app_properties(
  ladish_app_supervisor_proxy_handle proxy,
  uint64_t id,
  const char * name,
  const char * command,
  bool terminal,
  const char * level);

#endif /* #ifndef APP_SUPERVISOR_PROXY_H__A48C609D_0AB6_4C91_A9B0_BC7F1B7E4CB4__INCLUDED */
