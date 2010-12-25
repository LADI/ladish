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
#include "dynmenu.h"
#include "project_properties.h"

static GtkWidget * g_menu_item_new_studio;
static GtkWidget * g_menu_item_start_studio;
static GtkWidget * g_menu_item_stop_studio;
static GtkWidget * g_menu_item_save_studio;
static GtkWidget * g_menu_item_save_as_studio;
static GtkWidget * g_menu_item_unload_studio;
static GtkWidget * g_menu_item_rename_studio;
static GtkWidget * g_menu_item_create_room;
static GtkWidget * g_menu_item_destroy_room;
static GtkWidget * g_menu_item_project;
static GtkWidget * g_menu_item_ladishd_exit;
static GtkWidget * g_menu_item_jackdbus_exit;
static GtkWidget * g_menu_item_a2jmidid_exit;
static GtkWidget * g_menu_item_jack_configure;
static GtkWidget * g_menu_item_settings;
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

static ladish_dynmenu_handle g_project_dynmenu;

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

struct ladish_recent_projects_list_closure
{
  void
  (* callback)(
    void * context,
    const char * name,
    void * data,
    void (* item_activate_callback)(const char * name, void * data),
    void (* data_free)());
  void * context;
};

static void on_load_project(const char * name, void * data)
{
  log_info("Request to load project \"%s\":\"%s\"", name, (const char *)data);
  if (!ladish_room_proxy_load_project(graph_view_get_room(get_current_view()), data))
  {
    error_message_box(_("Project load failed, please inspect logs."));
  }
}

#define closure_ptr ((struct ladish_recent_projects_list_closure * )context)

static
void
add_recent_project(
  void * context,
  const char * project_name,
  const char * project_dir)
{
  closure_ptr->callback(closure_ptr->context, project_name, strdup(project_dir), NULL, free);
}

static
bool
fill_project_dynmenu(
  void (* callback)(
    void * context,
    const char * name,
    void * data,
    void (* item_activate_callback)(const char * name, void * data),
    void (* data_free)()),
  void * context)
{
  struct ladish_recent_projects_list_closure closure;
  bool has_project;
  graph_view_handle view;

  closure.callback = callback;
  closure.context = context;

  view = get_current_view();

  if (ladish_room_proxy_get_recent_projects(graph_view_get_room(view), 10, add_recent_project, &closure))
  {
    callback(context, NULL, NULL, NULL, NULL); /* add separator */
  }

  callback(context, _("Load Project..."), NULL, (ladish_dynmenu_item_activate_callback)menu_request_load_project, NULL);

  has_project = room_has_project(view);

  if (!has_project)
  {
    callback(context, _("Create Project..."), NULL, (ladish_dynmenu_item_activate_callback)menu_request_save_as_project, NULL);
  }

  callback(context, has_project ? _("Unload Project") : _("Clear Room"), NULL, (ladish_dynmenu_item_activate_callback)menu_request_unload_project, NULL);

  if (has_project)
  {
    callback(context, _("Save Project"), NULL, (ladish_dynmenu_item_activate_callback)menu_request_save_project, NULL);
    callback(context, _("Save Project As..."), NULL, (ladish_dynmenu_item_activate_callback)menu_request_save_as_project, NULL);
    callback(context, _("Project Properties..."), NULL, (ladish_dynmenu_item_activate_callback)ladish_project_properties_dialog_run, NULL);
  }

  return true;
}

#undef closure_ptr

bool menu_init(void)
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
  g_menu_item_project = get_gtk_builder_widget("project_menu_item");
  g_menu_item_ladishd_exit = get_gtk_builder_widget("menu_item_ladishd_exit");
  g_menu_item_jackdbus_exit = get_gtk_builder_widget("menu_item_jackdbus_exit");
  g_menu_item_a2jmidid_exit = get_gtk_builder_widget("menu_item_a2jmidid_exit");
  g_menu_item_jack_configure = get_gtk_builder_widget("menu_item_jack_configure");
  g_menu_item_settings = get_gtk_builder_widget("menu_item_settings");
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
  g_signal_connect(G_OBJECT(g_menu_item_ladishd_exit), "activate", G_CALLBACK(menu_request_ladishd_exit), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_jackdbus_exit), "activate", G_CALLBACK(menu_request_jackdbus_exit), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_a2jmidid_exit), "activate", G_CALLBACK(menu_request_a2jmidid_exit), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_jack_configure), "activate", G_CALLBACK(menu_request_jack_configure), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_settings), "activate", G_CALLBACK(menu_request_settings), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_start_app), "activate", G_CALLBACK(menu_request_start_app), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_create_room), "activate", G_CALLBACK(menu_request_create_room), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_destroy_room), "activate", G_CALLBACK(menu_request_destroy_room), NULL);

  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_32), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)32);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_64), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)64);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_128), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)128);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_256), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)256);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_512), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)512);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_1024), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)1024);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_2048), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)2048);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_4096), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)4096);
  g_signal_connect(G_OBJECT(g_menu_item_jack_latency_8192), "toggled", G_CALLBACK(buffer_size_change_request), (gpointer)8192);

  if (!ladish_dynmenu_create(
        "project_menu_item",
        "project_menu",
        fill_project_dynmenu,
        "project menu",
        on_load_project,
        &g_project_dynmenu))
  {
    return false;
  }

  return true;
}

void menu_uninit(void)
{
  ladish_dynmenu_destroy(g_project_dynmenu);
}

void menu_studio_state_changed(unsigned int studio_state)
{
  graph_view_handle view;

  gtk_widget_set_sensitive(g_menu_item_start_studio, studio_state == STUDIO_STATE_STOPPED);
  gtk_widget_set_sensitive(g_menu_item_stop_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_save_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_save_as_studio, studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_unload_studio, studio_state != STUDIO_STATE_UNLOADED);
  gtk_widget_set_sensitive(g_menu_item_rename_studio, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_start_app, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);
  gtk_widget_set_sensitive(g_menu_item_create_room, studio_state == STUDIO_STATE_STOPPED || studio_state == STUDIO_STATE_STARTED);

  view = get_current_view();
  gtk_widget_set_sensitive(g_menu_item_project, studio_state == STUDIO_STATE_STARTED && view != NULL && is_room_view(view));
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
  gtk_widget_set_sensitive(g_menu_item_project, room && get_studio_state() == STUDIO_STATE_STARTED);
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

void fill_view_popup_menu(GtkMenu * menu, graph_view_handle view)
{
  GtkWidget * menuitem;

  log_info("filling view menu...");

  if (graph_view_get_app_supervisor(view) != NULL)
  {
    menuitem = gtk_menu_item_new_with_label(_("New Application..."));
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_start_app, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  if (is_room_view(view))
  {
    menuitem = gtk_separator_menu_item_new(); /* separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    ladish_dynmenu_fill_external(g_project_dynmenu, menu);

    menuitem = gtk_separator_menu_item_new(); /* separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Destroy Room"));
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_destroy_room, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  else
  {
    menuitem = gtk_menu_item_new_with_label(_("Create Room..."));
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_create_room, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
}
