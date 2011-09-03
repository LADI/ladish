/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to jack session helper functionality
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

#ifndef JACK_SESSION_H__3C0F2ED2_7FAB_460F_A34F_4E3CAB6AC552__INCLUDED
#define JACK_SESSION_H__3C0F2ED2_7FAB_460F_A34F_4E3CAB6AC552__INCLUDED

#include "common.h"

bool
ladish_js_save_app(
  uuid_t app_uuid,
  const char * parent_dir,
  void * completion_context,
  void (* completion_callback)(
    void * completion_context,
    const char * commandline));

#endif /* #ifndef JACK_SESSION_H__3C0F2ED2_7FAB_460F_A34F_4E3CAB6AC552__INCLUDED */
