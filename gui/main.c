/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that implements main() and other top-level functionality
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
#include "canvas.h"
#include "../proxies/control_proxy.h"
#include "world_tree.h"
#include "graph_view.h"
#include "../common/catdup.h"
#include "../proxies/studio_proxy.h"
#include "create_room_dialog.h"
#include "menu.h"
#include "about.h"
#include "statusbar.h"
#include "action.h"
#include "studio.h"
#include "jack.h"

GtkWidget * g_main_win;
GtkWidget * g_toolbar;

void
set_main_window_title(
  graph_view_handle view)
{
  char * title;

  if (view != NULL)
  {
    title = catdup(get_view_name(view), " - LADI Session Handler");
    gtk_window_set_title(GTK_WINDOW(g_main_win), title);
    free(title);
  }
  else
  {
    gtk_window_set_title(GTK_WINDOW(g_main_win), "LADI Session Handler");
  }
}

void arrange(void)
{
  canvas_handle canvas;

  log_info("arrange request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_arrange(canvas);
  }
}

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
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  if (!canvas_init())
  {
    log_error("Canvas initialization failed.");
    return 1;
  }

  if (!init_gtk_builder())
  {
    return 1;
  }

  g_main_win = get_gtk_builder_widget("main_win");
  g_toolbar = get_gtk_builder_widget("toolbar");

  init_dialogs();

  init_studio_lists();

  init_statusbar();
  init_jack_widgets();

  create_room_dialog_init();

  world_tree_init();
  view_init();

  init_actions_and_accelerators();

  menu_init();
  buffer_size_clear();

  dbus_init();

  if (!init_jack())
  {
    return 1;
  }

  if (!control_proxy_init())
  {
    return 1;
  }

  if (!studio_proxy_init())
  {
    return 1;
  }

  set_studio_callbacks();
  set_room_callbacks();

  g_signal_connect(G_OBJECT(g_main_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_quit")), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_view_arrange")), "activate", G_CALLBACK(arrange), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_help_about")), "activate", G_CALLBACK(show_about), NULL);

  gtk_widget_show(g_main_win);

  gtk_main();

  studio_proxy_uninit();
  control_proxy_uninit();
  uninit_jack();
  dbus_uninit();
  create_room_dialog_uninit();
  uninit_gtk_builder();

  return 0;
}
