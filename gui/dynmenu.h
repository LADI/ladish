/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the dynamic menu related code
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

#ifndef DYNMENU_H__38C9F38E_2BB9_4934_9E2B_A7FC2087DDA5__INCLUDED
#define DYNMENU_H__38C9F38E_2BB9_4934_9E2B_A7FC2087DDA5__INCLUDED

#include "common.h"

typedef struct ladish_dynmenu_tag { int unused; } * ladish_dynmenu_handle;

bool
ladish_dynmenu_create(
  const char * menu_item,
  const char * menu,
  bool
  (* fill_callback)(
    void
    (* callback)(
      void * context,
      const char * name,
      void * data,
      void (* data_free)()),
    void * context),
  const char * description,
  void (* item_activate_callback)(const char * name, void * data),
  ladish_dynmenu_handle * dynmenu_handle_ptr);

void
ladish_dynmenu_destroy(
  ladish_dynmenu_handle dynmenu_handle);

#endif /* #ifndef DYNMENU_H__38C9F38E_2BB9_4934_9E2B_A7FC2087DDA5__INCLUDED */
