/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the studio list handling code
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

#include "internal.h"
#include "gtk_builder.h"
#include "../proxies/control_proxy.h"
#include "dynmenu.h"

static ladish_dynmenu_handle g_load_studio_list;
static ladish_dynmenu_handle g_delete_studio_list;

struct ladish_studio_list_closure
{
  void
  (* callback)(
    void * context,
    const char * name,
    void * data,
    ladish_dynmenu_item_activate_callback item_activate_callback,
    void (* data_free)(void * data));
  void * context;
};

#define closure_ptr ((struct ladish_studio_list_closure * )context)

static
void
add_item(
  void * context,
  const char * studio_name)
{
  closure_ptr->callback(closure_ptr->context, studio_name, NULL, NULL, NULL);
}

#undef closure_ptr

static
bool
fill_callback(
  void
  (* callback)(
    void * context,
    const char * name,
    void * data,
    ladish_dynmenu_item_activate_callback item_activate_callback,
    void (* data_free)(void * data)),
  void * context)
{
  struct ladish_studio_list_closure closure;

  closure.callback = callback;
  closure.context = context;

  return control_proxy_get_studio_list(add_item, &closure);
}

static void on_load_studio_wrapper(const char * name, void * data)
{
  ASSERT(data == NULL);
  on_load_studio(name);
}

static void on_delete_studio_wrapper(const char * name, void * data)
{
  ASSERT(data == NULL);
  on_delete_studio(name);
}

bool create_studio_lists(void)
{
  if (!ladish_dynmenu_create(
        "menu_item_load_studio",
        "load_studio_menu",
        fill_callback,
        "studio list",
        on_load_studio_wrapper,
        &g_load_studio_list))
  {
    return false;
  }

  if (!ladish_dynmenu_create(
        "menu_item_delete_studio",
        "delete_studio_menu",
        fill_callback,
        "studio list",
        on_delete_studio_wrapper,
        &g_delete_studio_list))
  {
    ladish_dynmenu_destroy(g_load_studio_list);
    return false;
  }

  return true;
}

void destroy_studio_lists(void)
{
  ladish_dynmenu_destroy(g_delete_studio_list);
  ladish_dynmenu_destroy(g_load_studio_list);
}
