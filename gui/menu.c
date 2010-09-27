/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains menu related code
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
#include "menu.h"
#include "gtk_builder.h"
#include "studio.h"

static GtkWidget * g_menu_item_new_studio;
static GtkWidget * g_menu_item_start_studio;
static GtkWidget * g_menu_item_stop_studio;
static GtkWidget * g_menu_item_save_studio;
static GtkWidget * g_menu_item_save_as_studio;
static GtkWidget * g_menu_item_unload_studio;
static GtkWidget * g_menu_item_rename_studio;
static GtkWidget * g_menu_item_create_room;
static GtkWidget * g_menu_item_destroy_room;
static GtkWidget * g_menu_item_load_project;
static GtkWidget * g_menu_item_unload_project;
static GtkWidget * g_menu_item_save_project;
static GtkWidget * g_menu_item_save_as_project;
static GtkWidget * g_menu_item_daemon_exit;
static GtkWidget * g_menu_item_jack_configure;
static GtkCheckMenuItem * g_menu_item_jack_latency_32;
static GtkCheckMenuItem * g_menu_item_jack_latency_64;
static GtkCheckMenuItem * g_menu_item_jack_latency_128;
static GtkCheckMenuItem * g_menu_item_jack_latency_256;
static GtkCheckMenuItem * g_menu_item_jack_latency_512;
static GtkCheckMenuItem * g_menu_item_jack_latency_1024;
static GtkCheckMenuItem * g_menu_item_jack_latency_2048;
static GtkCheckMenuItem * g_menu_item_jack_latency_4096;
static GtkCheckMenuItem * g_menu_item_jack_latency_8192;
static GtkWidget * g_menu_item_view_toolbar;
static GtkWidget * g_menu_item_view_raw_jack;
static GtkWidget * g_menu_item_start_app;

static bool g_latency_changing;

typedef void (* menu_request_toggle_func)(bool visible);

static void toggled(GtkCheckMenuItem * checkmenuitem, gpointer user_data)
{
  ((menu_request_toggle_func)user_data)(gtk_check_menu_item_get_active(checkmenuitem));
}

static void buffer_size_change_request(GtkCheckMenuItem * item_ptr, gpointer user_data)
{
  if (g_latency_changing)
  { /* skip activations because of gtk_check_menu_item_set_active() called from menu_set_jack_latency() */
    return;
  }

  if (!item_ptr->active)
  { /* skip radio button deactivations, we are interested only in activations */
    return;
  }

  menu_request_jack_latency_change((uint32_t)(guintptr)user_data);
}

void menu_init(void)
{
  g_menu_item_new_studio = get_gtk_builder_widget("menu_item_new_studio");
  g_menu_item_start_app = get_gtk_builder_widget("menu_item_start_app");
  g_menu_item_start_studio = get_gtk_builder_widget("menu_item_start_studio");
  g_menu_item_stop_studio = get_gtk_builder_widget("menu_item_stop_studio");
  g_menu_item_save_studio = get_gtk_builder_widget("menu_item_save_studio");
  g_menu_item_save_as_studio = get_gtk_builder_widget("menu_item_save_as_studio");
  g_menu_item_unload_studio = get_gtk_builder_widget("menu_item_unload_studio");
  g_menu_item_rename_studio = get_gtk_builder_widget("menu_item_rename_studio");
  g_menu_item_create_room = get_gtk_builder_widget("menu_item_create_room");
  g_menu_item_destroy_room = get_gtk_builder_widget("menu_item_destroy_room");
  g_menu_item_load_project = get_gtk_builder_widget("menu_item_load_project");
  g_menu_item_unload_project = get_gtk_builder_widget("menu_item_unload_project");
  g_menu_item_save_project = get_gtk_builder_widget("menu_item_save_project");
  g_menu_item_save_as_project = get_gtk_builder_widget("menu_item_save_as_project");
  g_menu_item_daemon_exit = get_gtk_builder_widget("menu_item_daemon_exit");
  g_menu_item_jack_configure = get_gtk_builder_widget("menu_item_jack_configure");
  g_menu_item_view_toolbar = get_gtk_builder_widget("menu_item_view_toolbar");
  g_menu_item_view_raw_jack = get_gtk_builder_widget("menu_item_view_raw_jack");

  g_menu_item_jack_latency_32   = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_32"));
  g_menu_item_jack_latency_64   = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_64"));
  g_menu_item_jack_latency_128  = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_128"));
  g_menu_item_jack_latency_256  = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_256"));
  g_menu_item_jack_latency_512  = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_512"));
  g_menu_item_jack_latency_1024 = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_1024"));
  g_menu_item_jack_latency_2048 = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_2048"));
  g_menu_item_jack_latency_4096 = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_4096"));
  g_menu_item_jack_latency_8192 = GTK_CHECK_MENU_ITEM(get_gtk_builder_widget("menu_item_jack_latency_8192"));

  g_signal_connect(G_OBJECT(g_menu_item_view_toolbar), "toggled", G_CALLBACK(toggled), menu_request_toggle_toolbar);
  g_signal_connect(G_OBJECT(g_menu_item_view_raw_jack), "toggled", G_CALLBACK(toggled), menu_request_toggle_raw_jack);

  g_signal_connect(G_OBJECT(g_menu_item_new_studio), "activate", G_CALLBACK(menu_request_new_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_start_studio), "activate", G_CALLBACK(menu_request_start_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_stop_studio), "activate", G_CALLBACK(menu_request_stop_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_unload_studio), "activate", G_CALLBACK(menu_request_unload_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_save_studio), "activate", G_CALLBACK(menu_request_save_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_save_as_studio), "activate", G_CALLBACK(menu_request_save_as_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_rename_studio), "activate", G_CALLBACK(menu_request_rename_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_daemon_exit), "activate", G_CALLBACK(menu_request_daemon_exit), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_jack_configure), "activate", G_CALLBACK(menu_request_jack_configure), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_start_app), "activate", G_CALLBACK(menu_request_start_app), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_create_room), "activate", G_CALLBACK(menu_request_create_room), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_destroy_room), "activate", G_CALLBACK(menu_request_destroy_room), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_load_project), "activate", G_CALLBACK(menu_request_load_project), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_unload_project), "activate", G_CALLBACK(menu_request_unload_project), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_save_project), "activate", G_CALLBACK(menu_request_save_project), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_save_as_project), "activate", G_CALLBACK(menu_request_save_as_project), NULL);

  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_32), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)32);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_64), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)64);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_128), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)128);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_256), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)256);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_512), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)512);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_1024), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)1024);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_2048), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)2048);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_4096), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)4096);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_8192), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)8192);
}

void menu_studio_state_changed(unsigned int studio_state)
{
  gtk_widget_set_sensitive(g_menu_item_start_studio, studio_state == STUDIO_STATE_STOPPED);
  gtk_widget_set_sensitive(g_menu_item_stop_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_save_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_save_as_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_unload_studio, studio_state != STUDIO_STATE_UNLOADED);
  gtk_widget_set_sensitive(g_menu_item_rename_studio, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_start_app, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_create_room, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);
}

void menu_set_jack_latency_items_sensivity(bool sensitive)
{
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_32), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_64), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_128), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_256), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_512), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_1024), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_2048), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_4096), sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(g_menu_item_jack_latency_8192), sensitive);
}

bool menu_set_jack_latency(uint32_t buffer_size, bool force)
{
  GtkCheckMenuItem * item_ptr;

  switch (buffer_size)
  {
  case 32:
    item_ptr = g_menu_item_jack_latency_32;
    break;
  case 64:
    item_ptr = g_menu_item_jack_latency_64;
    break;
  case 128:
    item_ptr = g_menu_item_jack_latency_128;
    break;
  case 256:
    item_ptr = g_menu_item_jack_latency_256;
    break;
  case 512:
    item_ptr = g_menu_item_jack_latency_512;
    break;
  case 1024:
    item_ptr = g_menu_item_jack_latency_1024;
    break;
  case 2048:
    item_ptr = g_menu_item_jack_latency_2048;
    break;
  case 4096:
    item_ptr = g_menu_item_jack_latency_4096;
    break;
  case 8192:
    item_ptr = g_menu_item_jack_latency_8192;
    break;
  default:
    //log_error("unknown jack buffer size %"PRIu32, buffer_size);
    return false;
  }

  if (force || !item_ptr->active)
  {
    log_info("menu_set_jack_latency() detects change");
    g_latency_changing = true;  /* latency has changed externally, don't tell jack to change it again */
    gtk_check_menu_item_set_active(item_ptr, TRUE);
    g_latency_changing = false;
  }

  return true;
}

void menu_set_toolbar_visibility(bool visible)
{
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_menu_item_view_toolbar), visible);
}

void menu_view_activated(bool room)
{
  gtk_widget_set_sensitive(g_menu_item_destroy_room, room);
  gtk_widget_set_sensitive(g_menu_item_load_project, room);
  gtk_widget_set_sensitive(g_menu_item_unload_project, room);
  gtk_widget_set_sensitive(g_menu_item_save_project, room);
  gtk_widget_set_sensitive(g_menu_item_save_as_project, room);
}

static void on_popup_menu_action_start_app(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_start_app();
}

static void on_popup_menu_action_create_room(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_create_room();
}

static void on_popup_menu_action_destroy_room(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_destroy_room();
}

static void on_popup_menu_action_load_project(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_load_project();
}

static void on_popup_menu_action_unload_project(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_unload_project();
}

static void on_popup_menu_action_save_project(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_save_project();
}

static void on_popup_menu_action_save_project_as(GtkWidget * menuitem, gpointer userdata)
{
  menu_request_save_as_project();
}

void fill_view_popup_menu(GtkMenu * menu, graph_view_handle view)
{
  GtkWidget * menuitem;

  log_info("filling view menu...");

  if (graph_view_get_app_supervisor(view) != NULL)
  {
    menuitem = gtk_menu_item_new_with_label("Run...");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_start_app, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  if (is_room_view(view))
  {
    menuitem = gtk_menu_item_new_with_label("Load Project...");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_load_project, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Unload Project");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_unload_project, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Save Project...");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_save_project, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Save Project As...");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_save_project_as, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_separator_menu_item_new(); /* separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Destroy Room");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_destroy_room, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  else
  {
    menuitem = gtk_menu_item_new_with_label("Create Room...");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_create_room, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
}
