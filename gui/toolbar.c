/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains toolbar related code
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

#include "toolbar.h"
#include "../proxies/conf_proxy.h"
#include "menu.h"
#include "gtk_builder.h"

#define LADISH_CONF_KEY_GLADISH_TOOLBAR_VISIBILITY "/org/ladish/gladish/toolbar_visibility"

static GtkWidget * g_toolbar;

void menu_request_toggle_toolbar(bool visible)
{
	if (visible)
  {
		gtk_widget_show(g_toolbar);
  }
	else
  {
		gtk_widget_hide(g_toolbar);
  }

  conf_set_bool(LADISH_CONF_KEY_GLADISH_TOOLBAR_VISIBILITY, visible);
}

void on_dbus_toggle_toobar(void * UNUSED(context), const char * UNUSED(key), const char * value)
{
  bool toolbar_visible;

  if (value == NULL)
  {
    toolbar_visible = false;
  }
  else
  {
    toolbar_visible = conf_string2bool(value);
  }

  if (toolbar_visible)
  {
    gtk_widget_show(g_toolbar);
  }
  else
  {
    gtk_widget_hide(g_toolbar);
  }

  menu_set_toolbar_visibility(toolbar_visible);
}

bool toolbar_init(void)
{
  g_toolbar = get_gtk_builder_widget("toolbar");

  if (!conf_register(LADISH_CONF_KEY_GLADISH_TOOLBAR_VISIBILITY, on_dbus_toggle_toobar, NULL))
  {
    return false;
  }

  return true;
}
