/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "common.h"

#include <math.h>

#include "glade.h"
#include "canvas.h"
#include "graph_canvas.h"
#include "../jack_proxy.h"
#include "dbus_helpers.h"
#include "control_proxy.h"
#include "../dbus_constants.h"
#include "world_tree.h"
#include "graph_view.h"
#include "../catdup.h"
#include "../studio_proxy.h"

GtkWidget * g_main_win;

GtkWidget * g_clear_load_button;
GtkWidget * g_xrun_progress_bar;
GtkWidget * g_buffer_size_combo;

GtkWidget * g_menu_item_load_studio;
GtkWidget * g_menu_item_save_studio;
GtkWidget * g_menu_item_rename_studio;
GtkWidget * g_menu_item_create_room;
GtkWidget * g_menu_item_destroy_room;
GtkWidget * g_menu_item_load_project;
GtkWidget * g_menu_item_start_app;
GtkWidget * g_menu_studio_list;

GtkWidget * g_rename_dialog;

graph_view_handle g_jack_view = NULL;
graph_view_handle g_studio_view = NULL;

static guint g_jack_poll_source_tag;
static double g_jack_max_dsp_load = 0.0;
static int g_studio_count = 0;

#if 0
static void
gtkmm_get_ink_pixel_size (Glib::RefPtr<Pango::Layout> layout,
             int& width,
             int& height)
{
  Pango::Rectangle ink_rect = layout->get_ink_extents ();
  
  width = (ink_rect.get_width() + PANGO_SCALE / 2) / PANGO_SCALE;
  height = (ink_rect.get_height() + PANGO_SCALE / 2) / PANGO_SCALE;
}

static void
gtkmm_set_width_for_given_text (Gtk::Widget &w, const gchar *text,
               gint hpadding/*, gint vpadding*/)
  
{
  int old_width, old_height;
  w.get_size_request(old_width, old_height);

  int width, height;
  w.ensure_style ();
  
  gtkmm_get_ink_pixel_size (w.create_pango_layout (text), width, height);
  w.set_size_request(width + hpadding, old_height);//height + vpadding);
}

#endif

static void set_buffer_size_combo_width(void)
{
  //gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);
}

static void buffer_size_clear()
{
  gtk_entry_set_text(GTK_ENTRY(get_glade_widget("comboboxentry")), "");
}

static void buffer_size_set(uint32_t size)
{
  gtk_combo_box_set_active(GTK_COMBO_BOX(g_buffer_size_combo), (int)log2f(size) - 5);
}

static void buffer_size_change_request(void)
{
  const int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(g_buffer_size_combo));

  if (selected < 0 || !jack_proxy_set_buffer_size(1 << (selected + 5)))
  {
    lash_error("cannot set JACK buffer size");
    buffer_size_clear();
  }
}

static void update_buffer_size(void)
{
  uint32_t size;

  if (jack_proxy_get_buffer_size(&size))
  {
    buffer_size_set(size);
  }
  else
  {
    buffer_size_clear();
  }
}

static void update_load(void)
{
  uint32_t xruns;
  double load;
  char tmp_buf[100];

  if (!jack_proxy_get_xruns(&xruns) || !jack_proxy_get_dsp_load(&load))
  {
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), "error");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
  }

  snprintf(tmp_buf, sizeof(tmp_buf), "%" PRIu32 " Dropouts", xruns);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), tmp_buf);

  load /= 100.0;           // dbus returns it in percents, we use 0..1

  if (load > g_jack_max_dsp_load)
  {
    g_jack_max_dsp_load = load;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), load);
  }
}

static void clear_load(void)
{
  jack_proxy_reset_xruns();
  g_jack_max_dsp_load = 0.0;
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
}

bool rename_dialog(const char * object, const char * old_name, char ** new_name)
{
  guint result;
  bool renamed;
  GtkEntry * entry = GTK_ENTRY(get_glade_widget("rename_entry"));

  gtk_widget_show(g_rename_dialog);

  gtk_label_set_text(GTK_LABEL(get_glade_widget("rename_label")), object);
  gtk_entry_set_text(entry, old_name);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

  result = gtk_dialog_run(GTK_DIALOG(g_rename_dialog));
  renamed = result == 2;
  if (renamed)
  {
    *new_name = strdup(gtk_entry_get_text(entry));
    if (*new_name == NULL)
    {
      lash_error("strdup failed for new name (rename)");
      renamed = false;
    }
  }

  gtk_widget_hide(g_rename_dialog);

  return renamed;
}

static void arrange(void)
{
  canvas_handle canvas;

  lash_info("arrange request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_arrange(canvas);
  }
}

static void on_load_studio(GtkWidget * item)
{
  const char * studio_name;

  studio_name = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))));
  lash_info("Load studio \"%s\"", studio_name);

  if (!control_proxy_load_studio(studio_name))
  {
    /* TODO: display error message */
  }
}

static void remove_studio_list_menu_entry(GtkWidget * item, GtkContainer * container)
{
  GtkWidget * label;

  label = gtk_bin_get_child(GTK_BIN(item));

  //lash_debug("removing studio menu item \"%s\"", gtk_menu_item_get_label(GTK_MENU_ITEM(item));
  // gtk_menu_item_get_label() requries gtk 2.16
  lash_debug("removing studio menu item \"%s\"", gtk_label_get_text(GTK_LABEL(label)));

  gtk_container_remove(GTK_CONTAINER(item), label); /* destroy the label and drop the item refcount by one */
  //lash_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count);
  gtk_container_remove(container, item);            /* drop the refcount of item by one and thus destroy it */
  g_studio_count--;
}

static void menu_studio_list_clear(void)
{
  gtk_container_foreach(GTK_CONTAINER(g_menu_studio_list), (GtkCallback)remove_studio_list_menu_entry, GTK_CONTAINER(g_menu_studio_list));
  assert(g_studio_count == 0);
  g_studio_count = 0;
}

static void add_studio_list_menu_entry(void * context, const char * studio_name)
{
  GtkWidget * item;

  item = gtk_menu_item_new_with_label(studio_name);
  //lash_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count); // refcount == 2 because of the label
  gtk_widget_set_sensitive(item, (bool)context);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(g_menu_studio_list), item);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_load_studio), item);
  g_studio_count++;
}

static void activate_load_studio_list(void)
{
  menu_studio_list_clear();
  if (!control_proxy_get_studio_list(add_studio_list_menu_entry, (void *)true))
  {
    menu_studio_list_clear();
    add_studio_list_menu_entry((void *)false, "Error obtaining studio list");
  }
  else if (g_studio_count == 0)
  {
    add_studio_list_menu_entry((void *)false, "Empty studio list");
  }
}

static void save_studio(void)
{
  lash_info("save studio request");
  if (!studio_proxy_save())
  {
    lash_error("studio save failed");
  }
}

static void rename_studio(void)
{
  char * new_name;

  if (rename_dialog("Studio name", get_view_name(g_studio_view), &new_name))
  {
    if (!studio_proxy_rename(new_name))
    {
      lash_error("studio rename failed");
    }

    free(new_name);
  }
}

static gboolean poll_jack(gpointer data)
{
  update_load();
  update_buffer_size();

  return TRUE;
}

void control_proxy_on_studio_appeared(void)
{
  char * name;

  if (!studio_proxy_get_name(&name))
  {
    lash_error("failed to get studio name");
    goto exit;
  }

  if (g_studio_view != NULL)
  {
    lash_error("studio appear signal received but studio already exists");
    goto free_name;
  }

  if (!create_view(name, SERVICE_NAME, STUDIO_OBJECT_PATH, false, &g_studio_view))
  {
    lash_error("create_view() failed for studio");
    goto free_name;
  }

  gtk_widget_set_sensitive(g_menu_item_save_studio, true);
  gtk_widget_set_sensitive(g_menu_item_rename_studio, true);
  gtk_widget_set_sensitive(g_menu_item_create_room, true);
  gtk_widget_set_sensitive(g_menu_item_destroy_room, true);
  gtk_widget_set_sensitive(g_menu_item_load_project, true);
  gtk_widget_set_sensitive(g_menu_item_start_app, true);

free_name:
  free(name);

exit:
  return;
}

void control_proxy_on_studio_disappeared(void)
{
  if (g_studio_view == NULL)
  {
    lash_error("studio disappear signal received but studio does not exists");
    return;
  }

  gtk_widget_set_sensitive(g_menu_item_save_studio, false);
  gtk_widget_set_sensitive(g_menu_item_rename_studio, false);
  gtk_widget_set_sensitive(g_menu_item_create_room, false);
  gtk_widget_set_sensitive(g_menu_item_destroy_room, false);
  gtk_widget_set_sensitive(g_menu_item_load_project, false);
  gtk_widget_set_sensitive(g_menu_item_start_app, false);

  if (g_studio_view != NULL)
  {
    destroy_view(g_studio_view);
    g_studio_view = NULL;
  }
}

static void on_studio_renamed(const char * new_studio_name)
{
  if (g_studio_view != NULL)
  {
    set_view_name(g_studio_view, new_studio_name);
  }
}

void jack_started(void)
{
  lash_info("JACK started");

  gtk_widget_set_sensitive(g_buffer_size_combo, true);
  gtk_widget_set_sensitive(g_clear_load_button, true);

  g_jack_poll_source_tag = g_timeout_add(100, poll_jack, NULL);
}

void jack_stopped(void)
{
  lash_info("JACK stopped");

  g_source_remove(g_jack_poll_source_tag);

  gtk_widget_set_sensitive(g_buffer_size_combo, false);
  buffer_size_clear();
  gtk_widget_set_sensitive(g_clear_load_button, false);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
}

void jack_appeared(void)
{
  lash_info("JACK appeared");

  if (!create_view("Raw JACK", JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, true, &g_jack_view))
  {
    lash_error("create_view() failed for jack");
    return;
  }
}

void jack_disappeared(void)
{
  lash_info("JACK disappeared");

  if (g_jack_view != NULL)
  {
    destroy_view(g_jack_view);
    g_jack_view = NULL;
  }
}

void
set_main_window_title(
  graph_view_handle view)
{
  char * title = catdup(get_view_name(view), " - LADI Session Handler");
  gtk_window_set_title(GTK_WINDOW(g_main_win), title);
  free(title);
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  if (!canvas_init())
  {
    lash_error("Canvas initialization failed.");
    return 1;
  }

  if (!init_glade())
  {
    return 1;
  }

  g_main_win = get_glade_widget("main_win");
  g_clear_load_button = get_glade_widget("clear_load_button");
  g_xrun_progress_bar = get_glade_widget("xrun_progress_bar");
  g_buffer_size_combo = get_glade_widget("buffer_size_combo");
  g_menu_item_load_studio = get_glade_widget("menu_item_load_studio");
  g_menu_item_save_studio = get_glade_widget("menu_item_save_studio");
  g_menu_item_rename_studio = get_glade_widget("menu_item_rename_studio");
  g_menu_item_create_room = get_glade_widget("menu_item_create_room");
  g_menu_item_destroy_room = get_glade_widget("menu_item_destroy_room");
  g_menu_item_load_project = get_glade_widget("menu_item_load_project");
  g_menu_item_start_app = get_glade_widget("menu_item_start_app");
  g_menu_studio_list = get_glade_widget("studio_list_menu");

  g_rename_dialog = get_glade_widget("rename_dialog");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(g_menu_item_load_studio), g_menu_studio_list);

  world_tree_init();
  view_init();

  patchage_dbus_init();

  if (!jack_proxy_init(jack_started, jack_stopped, jack_appeared, jack_disappeared))
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

  studio_proxy_set_renamed_callback(on_studio_renamed);

  set_buffer_size_combo_width();

  g_signal_connect(G_OBJECT(g_main_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_glade_widget("menu_item_quit")), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(g_buffer_size_combo), "changed", G_CALLBACK(buffer_size_change_request), NULL);
  g_signal_connect(G_OBJECT(g_clear_load_button), "clicked", G_CALLBACK(clear_load), NULL);
  g_signal_connect(G_OBJECT(get_glade_widget("menu_item_view_arrange")), "activate", G_CALLBACK(arrange), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_save_studio), "activate", G_CALLBACK(save_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_rename_studio), "activate", G_CALLBACK(rename_studio), NULL);
  g_signal_connect(G_OBJECT(g_menu_item_load_studio), "activate", G_CALLBACK(activate_load_studio_list), NULL);

  gtk_widget_show(g_main_win);

  //_about_win->set_transient_for(*_main_win);

  gtk_main();

  studio_proxy_uninit();
  control_proxy_uninit();
  uninit_glade();

  return 0;
}
