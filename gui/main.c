/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "internal.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gtk_builder.h"
#include "canvas.h"
#include "graph_canvas.h"
#include "../proxies/jack_proxy.h"
#include "../proxies/control_proxy.h"
#include "../dbus_constants.h"
#include "world_tree.h"
#include "graph_view.h"
#include "../catdup.h"
#include "../proxies/studio_proxy.h"
#include "ask_dialog.h"
#include "../proxies/app_supervisor_proxy.h"
#include "create_room_dialog.h"
#include "menu.h"

GtkWidget * g_main_win;

GtkAction * g_clear_xruns_and_max_dsp_action;
GtkStatusbar * g_statusbar;

GtkWidget * g_studio_status_label;
GtkWidget * g_toolbar;
GtkWidget * g_status_image;

GtkWidget * g_name_dialog;
GtkWidget * g_app_dialog;

GtkWidget * g_sample_rate_label;
uint32_t g_sample_rate;

GtkWidget * g_latency_label;
GtkWidget * g_dsp_load_label;
GtkWidget * g_xruns_label;

GtkWidget * g_xrun_progress_bar;

graph_view_handle g_jack_view = NULL;
graph_view_handle g_studio_view = NULL;

static guint g_jack_poll_source_tag;
static guint g_ladishd_poll_source_tag;
static double g_jack_max_dsp_load = 0.0;

static unsigned int g_studio_state = STUDIO_STATE_UNKNOWN;

#define JACK_STATE_NA         0
#define JACK_STATE_STOPPED    1
#define JACK_STATE_STARTED    2

static unsigned int g_jack_state = JACK_STATE_NA;

#define ABOUT_DIALOG_LOGO     "ladish-logo-128x128.png"
#define STATUS_ICON_DOWN      "status_down.png"         /* temporary down during service restart */
#define STATUS_ICON_UNLOADED  "status_unloaded.png"
#define STATUS_ICON_STARTED   "status_started.png"
#define STATUS_ICON_STOPPED   "status_stopped.png"
#define STATUS_ICON_WARNING   "status_warning.png"      /* xruns */
#define STATUS_ICON_ERROR     "status_error.png"        /* bad error */

static uint32_t g_xruns;

struct studio_list
{
  int count;
  GtkWidget * menu_item;
  GtkWidget * menu;
  void (* item_activate_callback)(GtkWidget * item);
  bool add_sensitive;
};

static struct studio_list g_load_studio_list;
static struct studio_list g_delete_studio_list;

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

static GdkPixbuf * load_pixbuf_internal(const char * directory, const char * filename)
{
  char * fullpath;
  GdkPixbuf * pixbuf;

  fullpath = catdup(directory, filename);
  if (fullpath == NULL)
  {
    return NULL;
  }

  pixbuf = gdk_pixbuf_new_from_file(fullpath, NULL);

  free(fullpath);

  return pixbuf;
}

static GdkPixbuf * load_pixbuf(const char * filename)
{
  GdkPixbuf * pixbuf;
  static const char * pixbuf_dirs[] = {"./art/", DATA_DIR "/", NULL};
  const char ** dir;

  for (dir = pixbuf_dirs; *dir != NULL; dir++)
  {
    pixbuf = load_pixbuf_internal(*dir, filename);
    if (pixbuf != NULL)
    {
      return pixbuf;
    }
  }

  return NULL;
}

bool studio_state_changed(char ** name_ptr_ptr)
{
  const char * status;
  const char * name;
  char * buffer;
  const char * status_image_path;
  const char * tooltip;
  GdkPixbuf * pixbuf;

  menu_studio_state_changed(g_studio_state);

  tooltip = NULL;
  status_image_path = NULL;

  switch (g_jack_state)
  {
  case JACK_STATE_NA:
    tooltip = status = "JACK is sick";
    status_image_path = STATUS_ICON_ERROR;
    break;
  case JACK_STATE_STOPPED:
    status = "Stopped";
    break;
  case JACK_STATE_STARTED:
    status = "xruns";
    break;
  default:
    status = "???";
    tooltip = "Internal error - unknown jack state";
    status_image_path = STATUS_ICON_ERROR;
  }

  buffer = NULL;

  switch (g_studio_state)
  {
  case STUDIO_STATE_NA:
    name = "ladishd is down";
    status_image_path = STATUS_ICON_DOWN;
    break;
  case STUDIO_STATE_SICK:
  case STUDIO_STATE_UNKNOWN:
    tooltip = name = "ladishd is sick";
    status_image_path = STATUS_ICON_ERROR;
    break;
  case STUDIO_STATE_UNLOADED:
    name = "No studio loaded";
    status_image_path = STATUS_ICON_UNLOADED;
    break;
  case STUDIO_STATE_CRASHED:
    status = "Crashed";
    tooltip = "Crashed studio, save your work if you can and unload the studio";
    status_image_path = STATUS_ICON_ERROR;
    /* fall through */
  case STUDIO_STATE_STOPPED:
  case STUDIO_STATE_STARTED:
    if (!studio_proxy_get_name(&buffer))
    {
      tooltip = "failed to get studio name";
      log_error("%s", tooltip);
      status_image_path = STATUS_ICON_ERROR;
    }
    else
    {
      name = buffer;
      switch (g_studio_state)
      {
      case STUDIO_STATE_STARTED:
        status_image_path = g_xruns > 0 ? STATUS_ICON_WARNING : STATUS_ICON_STARTED;
        tooltip = "Studio is started";
        break;
      case STUDIO_STATE_STOPPED:
        status_image_path = STATUS_ICON_STOPPED;
        tooltip = "Studio is stopped";
        break;
      }
      break;
    }
  default:
    name = "???";
    tooltip = "Internal error - unknown studio state";
    status_image_path = STATUS_ICON_ERROR;
  }

  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), status);
  gtk_label_set_text(GTK_LABEL(g_studio_status_label), name);

  if (status_image_path == NULL || (pixbuf = load_pixbuf(status_image_path)) == NULL)
  {
    gtk_image_set_from_stock(GTK_IMAGE(g_status_image), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  }
  else
  {
    gtk_image_set_from_pixbuf(GTK_IMAGE(g_status_image), pixbuf);
    g_object_unref(pixbuf);
  }

  //gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(g_status_tool_item), tooltip);

  if (g_jack_state == JACK_STATE_STARTED)
  {
    if (jack_proxy_sample_rate(&g_sample_rate))
    {
      char buf[100];

      if (fmod(g_sample_rate, 1000.0) != 0.0)
      {
        snprintf(buf, sizeof(buf), "%.1f kHz", (float)g_sample_rate / 1000.0f);
      }
      else
      {
        snprintf(buf, sizeof(buf), "%u kHz", g_sample_rate / 1000);
      }

      gtk_label_set_text(GTK_LABEL(g_sample_rate_label), buf);
    }
    else
    {
      gtk_label_set_text(GTK_LABEL(g_sample_rate_label), "");
    }
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(g_sample_rate_label), "");
    gtk_label_set_text(GTK_LABEL(g_latency_label), "");
    gtk_label_set_text(GTK_LABEL(g_dsp_load_label), "");
    gtk_label_set_text(GTK_LABEL(g_xruns_label), "");
  }

  if (buffer == NULL)
  {
    return false;
  }

  if (name_ptr_ptr != NULL)
  {
    *name_ptr_ptr = buffer;
  }
  else
  {
    free(buffer);
  }

  return true;
}

static void buffer_size_clear(void)
{
  menu_set_jack_latency_items_sensivity(false);
  gtk_label_set_text(GTK_LABEL(g_latency_label), "");
}

static void buffer_size_set(uint32_t size, bool force)
{
  bool changed;
  char buf[100];

  if (!menu_set_jack_latency(size, force, &changed))
  {
    buffer_size_clear();
    return;
  }

  if (changed)
  {
    log_info("JACK latency changed: %"PRIu32" samples", size);

    snprintf(buf, sizeof(buf), "%4.1f ms (%"PRIu32")", (float)size / (float)g_sample_rate * 1000.0f, size);
    gtk_label_set_text(GTK_LABEL(g_latency_label), buf);
  }
}

void menu_request_jack_latency_change(uint32_t buffer_size)
{
  log_info("JACK latency change request: %"PRIu32" samples", buffer_size);

  if (!jack_proxy_set_buffer_size(buffer_size))
  {
    log_error("cannot set JACK buffer size");
  }
}

static void update_buffer_size(bool force)
{
  uint32_t size;

  if (jack_proxy_get_buffer_size(&size))
  {
    buffer_size_set(size, force);
  }
  else
  {
    buffer_size_clear();
  }
}

static void update_load(void)
{
  double load;
  char tmp_buf[100];
  uint32_t xruns;

  if (jack_proxy_get_xruns(&xruns))
  {
    snprintf(tmp_buf, sizeof(tmp_buf), "%" PRIu32 " dropouts", xruns);
    gtk_label_set_text(GTK_LABEL(g_xruns_label), tmp_buf);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), tmp_buf);
  }
  else
  {
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), "error");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
    gtk_label_set_text(GTK_LABEL(g_xruns_label), "?");
  }

  if (jack_proxy_get_dsp_load(&load))
  {
    if (load > g_jack_max_dsp_load)
    {
      g_jack_max_dsp_load = load;
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), load / 100.0);
    }

    snprintf(tmp_buf, sizeof(tmp_buf), "DSP: %5.1f%% (%5.1f%%)", (float)load, (float)g_jack_max_dsp_load);
    gtk_label_set_text(GTK_LABEL(g_dsp_load_label), tmp_buf);
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(g_xruns_label), "?");
  }

  if ((g_xruns == 0 && xruns != 0) || (g_xruns != 0 && xruns == 0))
  {
    g_xruns = xruns;
    studio_state_changed(NULL);
  }
  else
  {
    g_xruns = xruns;
  }
}

static void clear_xruns_and_max_dsp(void)
{
  log_info("clearing xruns and max dsp load");
  jack_proxy_reset_xruns();
  g_jack_max_dsp_load = 0.0;
}

bool name_dialog(const char * title, const char * object, const char * old_name, char ** new_name)
{
  guint result;
  bool ok;
  GtkEntry * entry = GTK_ENTRY(get_gtk_builder_widget("name_entry"));

  gtk_window_set_title(GTK_WINDOW(g_app_dialog), title);

  gtk_widget_show(g_name_dialog);

  gtk_label_set_text(GTK_LABEL(get_gtk_builder_widget("name_label")), object);
  gtk_entry_set_text(entry, old_name);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

  result = gtk_dialog_run(GTK_DIALOG(g_name_dialog));
  ok = result == 2;
  if (ok)
  {
    *new_name = strdup(gtk_entry_get_text(entry));
    if (*new_name == NULL)
    {
      log_error("strdup failed for new name (name_dialog)");
      ok = false;
    }
  }

  gtk_widget_hide(g_name_dialog);

  return ok;
}

void error_message_box(const char * failed_operation)
{
  GtkWidget * dialog;
  dialog = get_gtk_builder_widget("error_dialog");
  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), "<b><big>Error</big></b>");
  gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", failed_operation);
  gtk_widget_show(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
}

void run_custom_command_dialog(void)
{
  graph_view_handle view;
  guint result;
  GtkEntry * command_entry = GTK_ENTRY(get_gtk_builder_widget("app_command_entry"));
  GtkEntry * name_entry = GTK_ENTRY(get_gtk_builder_widget("app_name_entry"));
  GtkToggleButton * terminal_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_terminal_check_button"));
  GtkToggleButton * level0_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level0"));
  GtkToggleButton * level1_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level1"));
  GtkToggleButton * level2_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level2"));
  GtkToggleButton * level3_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level3"));
  uint8_t level;

  view = get_current_view();

  gtk_entry_set_text(name_entry, "");
  gtk_entry_set_text(command_entry, "");
  gtk_toggle_button_set_active(terminal_button, FALSE);

  gtk_widget_set_sensitive(GTK_WIDGET(command_entry), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(terminal_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level0_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level1_button), TRUE);

  gtk_window_set_focus(GTK_WINDOW(g_app_dialog), GTK_WIDGET(command_entry));
  gtk_window_set_title(GTK_WINDOW(g_app_dialog), "New application");

  gtk_widget_show(g_app_dialog);

  result = gtk_dialog_run(GTK_DIALOG(g_app_dialog));
  if (result == 2)
  {
    if (gtk_toggle_button_get_active(level0_button))
    {
      level = 0;
    }
    else if (gtk_toggle_button_get_active(level1_button))
    {
      level = 1;
    }
    else if (gtk_toggle_button_get_active(level2_button))
    {
      level = 2;
    }
    else if (gtk_toggle_button_get_active(level3_button))
    {
      level = 3;
    }
    else
    {
      log_error("unknown level");
      ASSERT_NO_PASS;
      level = 0;
    }

    log_info("'%s':'%s' %s level %"PRIu8, gtk_entry_get_text(name_entry), gtk_entry_get_text(command_entry), gtk_toggle_button_get_active(terminal_button) ? "terminal" : "shell", level);
    if (!app_run_custom(
          view,
          gtk_entry_get_text(command_entry),
          gtk_entry_get_text(name_entry),
          gtk_toggle_button_get_active(terminal_button),
          level))
    {
      error_message_box("Execution failed. I know you want to know more for the reson but currently you can only check the log file.");
    }
  }

  gtk_widget_hide(g_app_dialog);
}

static void arrange(void)
{
  canvas_handle canvas;

  log_info("arrange request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_arrange(canvas);
  }
}

void menu_request_daemon_exit(void)
{
  log_info("Daemon exit request");

  if (!control_proxy_exit())
  {
    error_message_box("Daemon exit command failed, please inspect logs.");
  }
}

void menu_request_jack_configure(void)
{
  GError * error_ptr;
  gchar * argv[] = {"ladiconf", NULL};
  GtkWidget * dialog;

  log_info("JACK configure request");

  error_ptr = NULL;
  if (!g_spawn_async(
        NULL,                   /* working directory */
        argv,
        NULL,                   /* envp */
        G_SPAWN_SEARCH_PATH,    /* flags */
        NULL,                   /* child_setup callback */
        NULL,                   /* user_data */
        NULL,
        &error_ptr))
  {
    dialog = get_gtk_builder_widget("error_dialog");
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), "<b><big>Error executing ladiconf.\nAre LADI Tools installed?</big></b>");
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", error_ptr->message);
    gtk_widget_show(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    g_error_free(error_ptr);
  }
}

static void on_load_studio(GtkWidget * item)
{
  const char * studio_name;

  studio_name = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))));
  log_info("Load studio \"%s\"", studio_name);

  if (!control_proxy_load_studio(studio_name))
  {
    error_message_box("Studio load failed, please inspect logs.");
  }
}

static void on_delete_studio(GtkWidget * item)
{
  const char * studio_name;
  bool result;

  studio_name = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))));

  if (!ask_dialog(&result, "<b><big>Confirm studio delete</big></b>", "Studio \"%s\" will be deleted. Are you sure?", studio_name) || !result)
  {
    return;
  }

  log_info("Delete studio \"%s\"", studio_name);

  if (!control_proxy_delete_studio(studio_name))
  {
    error_message_box("Studio delete failed, please inspect logs.");
  }
}

#define studio_list_ptr ((struct studio_list *)context)

static void remove_studio_list_menu_entry(GtkWidget * item, gpointer context)
{
  GtkWidget * label;

  label = gtk_bin_get_child(GTK_BIN(item));

  //log_debug("removing studio menu item \"%s\"", gtk_menu_item_get_label(GTK_MENU_ITEM(item));
  // gtk_menu_item_get_label() requries gtk 2.16
  log_debug("removing studio menu item \"%s\"", gtk_label_get_text(GTK_LABEL(label)));

  gtk_container_remove(GTK_CONTAINER(item), label); /* destroy the label and drop the item refcount by one */
  //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count);
  gtk_container_remove(GTK_CONTAINER(studio_list_ptr->menu), item); /* drop the refcount of item by one and thus destroy it */
  studio_list_ptr->count--;
}

static void add_studio_list_menu_entry(void * context, const char * studio_name)
{
  GtkWidget * item;

  item = gtk_menu_item_new_with_label(studio_name);
  //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count); // refcount == 2 because of the label
  gtk_widget_set_sensitive(item, studio_list_ptr->add_sensitive);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(studio_list_ptr->menu), item);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(studio_list_ptr->item_activate_callback), item);
  studio_list_ptr->count++;
}

#undef studio_list_ptr

static void menu_studio_list_clear(struct studio_list * studio_list_ptr)
{
  gtk_container_foreach(GTK_CONTAINER(studio_list_ptr->menu), remove_studio_list_menu_entry, studio_list_ptr);
  ASSERT(studio_list_ptr->count == 0);
  studio_list_ptr->count = 0;
}

static void populate_studio_list_menu(GtkMenuItem * menu_item, struct studio_list * studio_list_ptr)
{
  menu_studio_list_clear(studio_list_ptr);
  studio_list_ptr->add_sensitive = true;
  if (!control_proxy_get_studio_list(add_studio_list_menu_entry, studio_list_ptr))
  {
    menu_studio_list_clear(studio_list_ptr);
    studio_list_ptr->add_sensitive = false;
    add_studio_list_menu_entry(studio_list_ptr, "Error obtaining studio list");
  }
  else if (studio_list_ptr->count == 0)
  {
    studio_list_ptr->add_sensitive = false;
    add_studio_list_menu_entry(studio_list_ptr, "Empty studio list");
  }
}

void menu_request_save_studio(void)
{
  log_info("save studio request");
  if (!studio_proxy_save())
  {
    error_message_box("Studio save failed, please inspect logs.");
  }
}

void menu_request_save_as_studio(void)
{
  char * new_name;

  log_info("save as studio request");

  if (name_dialog("Save studio as", "Studio name", "", &new_name))
  {
    if (!studio_proxy_save_as(new_name))
    {
      error_message_box("Saving of studio failed, please inspect logs.");
    }

    free(new_name);
  }
}

void menu_request_new_studio(void)
{
  char * new_name;

  log_info("new studio request");

  if (name_dialog("New studio", "Studio name", "", &new_name))
  {
    if (!control_proxy_new_studio(new_name))
    {
      error_message_box("Creation of new studio failed, please inspect logs.");
    }

    free(new_name);
  }
}

void menu_request_start_app(void)
{
  run_custom_command_dialog();
}

void menu_request_start_studio(void)
{
  log_info("start studio request");
  if (!studio_proxy_start())
  {
    error_message_box("Studio start failed, please inspect logs.");
  }
}

void menu_request_stop_studio(void)
{
  log_info("stop studio request");
  if (!studio_proxy_stop())
  {
    error_message_box("Studio stop failed, please inspect logs.");
  }
}

void menu_request_unload_studio(void)
{
  log_info("unload studio request");
  if (!studio_proxy_unload())
  {
    error_message_box("Studio unload failed, please inspect logs.");
  }
}

void menu_request_rename_studio(void)
{
  char * new_name;

  if (name_dialog("Rename studio", "Studio name", get_view_name(g_studio_view), &new_name))
  {
    if (!studio_proxy_rename(new_name))
    {
      error_message_box("Studio rename failed, please inspect logs.");
    }

    free(new_name);
  }
}

void menu_request_create_room(void)
{
  char * name;
  char * template;

  log_info("create room request");

  if (create_room_dialog_run(&name, &template))
  {
    log_info("Creating new room '%s' from template '%s'", name, template);

    if (!studio_proxy_create_room(name, template))
    {
      error_message_box("Room creation failed, please inspect logs.");
    }

    free(name);
    free(template);
  }
}

void menu_request_destroy_room(void)
{
  const char * room;

  room = get_current_view_room_name();
  if (room == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  log_info("destroy room '%s' request", room);

  if (!studio_proxy_delete_room(room))
  {
    error_message_box("Room deletion failed, please inspect logs.");
  }
}

static gboolean poll_jack(gpointer data)
{
  update_load();
  update_buffer_size(false);

  return TRUE;
}

static gboolean poll_ladishd(gpointer data)
{
  control_proxy_ping();
  return TRUE;
}

void control_proxy_on_daemon_appeared(void)
{
  if (g_studio_state == STUDIO_STATE_NA || g_studio_state == STUDIO_STATE_SICK)
  {
    log_info("ladishd appeared");
    g_source_remove(g_ladishd_poll_source_tag);
  }

  g_studio_state = STUDIO_STATE_UNLOADED;
  studio_state_changed(NULL);
}

void control_proxy_on_daemon_disappeared(bool clean_exit)
{
  log_info("ladishd disappeared");

  if (!clean_exit)
  {
    error_message_box("ladish daemon crashed");
    g_studio_state = STUDIO_STATE_SICK;
  }
  else
  {
    g_studio_state = STUDIO_STATE_NA;
  }

  studio_state_changed(NULL);

  if (g_studio_view != NULL)
  {
    destroy_view(g_studio_view);
    g_studio_view = NULL;
  }

  g_ladishd_poll_source_tag = g_timeout_add(500, poll_ladishd, NULL);
}

void control_proxy_on_studio_appeared(bool initial)
{
  char * name;
  bool started;

  g_studio_state = STUDIO_STATE_STOPPED;

  if (initial)
  {
    if (!studio_proxy_is_started(&started))
    {
      log_error("intially, studio is present but is_started() check failed.");
      return;
    }

    if (started)
    {
      g_studio_state = STUDIO_STATE_STARTED;
    }
  }

  if (studio_state_changed(&name))
  {
    if (g_studio_view != NULL)
    {
      log_error("studio appear signal received but studio already exists");
    }
    else if (!create_view(name, SERVICE_NAME, STUDIO_OBJECT_PATH, true, true, false, &g_studio_view))
    {
      log_error("create_view() failed for studio");
    }

    free(name);
  }
}

void control_proxy_on_studio_disappeared(void)
{
  g_studio_state = STUDIO_STATE_UNLOADED;
  studio_state_changed(NULL);

  if (g_studio_view == NULL)
  {
    log_error("studio disappear signal received but studio does not exists");
    return;
  }

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
    gtk_label_set_text(GTK_LABEL(g_studio_status_label), new_studio_name);
  }
}

void on_studio_started(void)
{
  g_studio_state = STUDIO_STATE_STARTED;
  studio_state_changed(NULL);
}

void on_studio_stopped(void)
{
  g_studio_state = STUDIO_STATE_STOPPED;
  studio_state_changed(NULL);
}

void on_studio_crashed(void)
{
  g_studio_state = STUDIO_STATE_CRASHED;
  studio_state_changed(NULL);
  error_message_box("JACK crashed or stopped unexpectedly. Save your work, then unload and reload the studio.");
}

void jack_started(void)
{
  log_info("JACK started");

  g_jack_state = JACK_STATE_STARTED;
  studio_state_changed(NULL);

  menu_set_jack_latency_items_sensivity(true);
  update_buffer_size(true);
  gtk_action_set_sensitive(g_clear_xruns_and_max_dsp_action, true);

  g_jack_poll_source_tag = g_timeout_add(100, poll_jack, NULL);
}

void jack_stopped(void)
{
  if (g_jack_state == JACK_STATE_STARTED)
  {
    log_info("JACK stopped");

    g_source_remove(g_jack_poll_source_tag);
  }

  g_jack_state = JACK_STATE_STOPPED;
  studio_state_changed(NULL);

  menu_set_jack_latency_items_sensivity(false);
  buffer_size_clear();
  gtk_action_set_sensitive(g_clear_xruns_and_max_dsp_action, false);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
}

void jack_appeared(void)
{
  log_info("JACK appeared");

  g_jack_state = JACK_STATE_STOPPED;
  studio_state_changed(NULL);

#if defined(SHOW_RAW_JACK)
  if (!create_view("Raw JACK", JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, false, false, true, &g_jack_view))
  {
    log_error("create_view() failed for jack");
    return;
  }
#endif
}

void jack_disappeared(void)
{
  log_info("JACK disappeared");

  jack_stopped();

  g_jack_state = JACK_STATE_NA;
  studio_state_changed(NULL);

#if defined(SHOW_RAW_JACK)
  if (g_jack_view != NULL)
  {
    destroy_view(g_jack_view);
    g_jack_view = NULL;
  }
#endif
}

static void room_appeared(const char * opath, const char * name, const char * template)
{
  graph_view_handle graph_view;

  log_info("room \"%s\" appeared (%s). template is \"%s\"", name, opath, template);

  if (!create_view(name, SERVICE_NAME, opath, true, true, false, &graph_view))
  {
    log_error("create_view() failed for room \"%s\"", name);
  }
}

static void room_disappeared(const char * opath, const char * name, const char * template)
{
  graph_view_handle graph_view;

  log_info("room \"%s\" disappeared (%s). template is \"%s\"", name, opath, template);

  graph_view = world_tree_find_by_opath(opath);
  if (graph_view == NULL)
  {
    log_error("Unknown room disappeared");
    return;
  }

  destroy_view(graph_view);
}

static void room_changed(const char * opath, const char * name, const char * template)
{
  log_info("%s changed. name is \"%s\". template is \"%s\"", opath, name, template);
}

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

static
void
init_studio_list(
  struct studio_list * studio_list_ptr,
  const char * menu_item,
  const char * menu,
  void (* item_activate_callback)(GtkWidget * item))
{
  studio_list_ptr->count = 0;
  studio_list_ptr->menu_item = get_gtk_builder_widget(menu_item);
  studio_list_ptr->menu = get_gtk_builder_widget(menu);
  studio_list_ptr->item_activate_callback = item_activate_callback;
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(studio_list_ptr->menu_item), studio_list_ptr->menu);
  g_signal_connect(G_OBJECT(studio_list_ptr->menu_item), "activate", G_CALLBACK(populate_studio_list_menu), studio_list_ptr);
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

static char * read_file_contents(const char * filename)
{
  int fd;
  struct stat st;
  char * buffer;

  if (stat(filename, &st) != 0)
  {
    return NULL;
  }

  fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    return NULL;
  }

  buffer = malloc(st.st_size + 1);
  if (buffer == NULL)
  {
    close(fd);
    return NULL;
  }

  if (read(fd, buffer, (size_t)st.st_size) != (ssize_t)st.st_size)
  {
    free(buffer);
    buffer = NULL;
  }
  else
  {
    buffer[st.st_size] = 0;
  }

  close(fd);

  return buffer;
}

static void show_about(void)
{
  GtkWidget * dialog;
  GdkPixbuf * pixbuf;
  const char * authors[] = {"Nedko Arnaudov", NULL};
  const char * artists[] = {"Lapo Calamandrei", "Nadejda Pancheva-Arnaudova", NULL};
  char * license;

  pixbuf =  load_pixbuf(ABOUT_DIALOG_LOGO);
  license = read_file_contents(DATA_DIR "/COPYING");

  dialog = get_gtk_builder_widget("about_win");
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), PACKAGE_VERSION);

  if (pixbuf != NULL)
  {
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
    g_object_unref(pixbuf);
  }

  if (license != NULL)
  {
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), license);
    free(license);
  }

  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
  gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), artists);

  gtk_widget_show(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
}

static void dbus_init(void)
{
  dbus_error_init(&g_dbus_error);

  // Connect to the bus
  g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &g_dbus_error);
  if (dbus_error_is_set(&g_dbus_error))
  {
    //error_msg("dbus_bus_get() failed");
    //error_msg(g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
  }

  dbus_connection_setup_with_g_main(g_dbus_connection, NULL);
}

void dbus_uninit(void)
{
  if (g_dbus_connection)
  {
    dbus_connection_flush(g_dbus_connection);
  }

  if (dbus_error_is_set(&g_dbus_error))
  {
    dbus_error_free(&g_dbus_error);
  }
}

void setup_accelerators(void)
{
  static GtkActionGroup * action_group_ptr;
  static GtkAccelGroup * accel_group_ptr;
  struct
  {
    GtkAction * action_ptr;
    const char * shortcut;
  } * descriptor_ptr, descriptors [] =
      {
        {g_clear_xruns_and_max_dsp_action, "c"},
        {NULL, NULL}
      };

  action_group_ptr = gtk_action_group_new("main");
  accel_group_ptr = gtk_accel_group_new();

  for (descriptor_ptr = descriptors; descriptor_ptr->action_ptr != NULL; descriptor_ptr++)
  {
    //log_info("action '%s' -> shortcut \"%s\"", gtk_action_get_name(descriptor_ptr->action_ptr), descriptor_ptr->shortcut);
    gtk_action_group_add_action_with_accel(action_group_ptr, descriptor_ptr->action_ptr, descriptor_ptr->shortcut);
    gtk_action_set_accel_group(descriptor_ptr->action_ptr, accel_group_ptr);
    gtk_action_connect_accelerator(descriptor_ptr->action_ptr);
  }

  gtk_window_add_accel_group(GTK_WINDOW(g_main_win), accel_group_ptr);
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
  g_clear_xruns_and_max_dsp_action = GTK_ACTION(get_gtk_builder_object("clear_xruns_and_max_dsp_load_action"));
  g_toolbar = get_gtk_builder_widget("toolbar");
  g_statusbar = GTK_STATUSBAR(get_gtk_builder_widget("statusbar"));
  g_xrun_progress_bar = get_gtk_builder_widget("xrun_progress_bar");

  g_name_dialog = get_gtk_builder_widget("name_dialog");
  g_app_dialog = get_gtk_builder_widget("app_dialog");

  init_studio_list(&g_load_studio_list, "menu_item_load_studio", "load_studio_menu", on_load_studio);
  init_studio_list(&g_delete_studio_list, "menu_item_delete_studio", "delete_studio_menu", on_delete_studio);

  g_studio_status_label = gtk_label_new("studioname");
  gtk_widget_show(g_studio_status_label);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_studio_status_label, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_studio_status_label, 0);

  g_status_image = gtk_image_new();
  gtk_widget_show(g_status_image);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_status_image, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_status_image, 1);

  g_sample_rate_label = gtk_label_new("srate");
  gtk_widget_show(g_sample_rate_label);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_sample_rate_label, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_sample_rate_label, 2);

  g_latency_label = gtk_label_new("latency");
  gtk_widget_show(g_latency_label);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_latency_label, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_latency_label, 3);

  g_xruns_label = gtk_label_new("xruns");
  gtk_widget_show(g_xruns_label);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_xruns_label, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_xruns_label, 4);

  g_dsp_load_label = gtk_label_new("load");
  gtk_widget_show(g_dsp_load_label);
  gtk_box_pack_start(GTK_BOX(g_statusbar), g_dsp_load_label, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), g_dsp_load_label, 5);

  create_room_dialog_init();

  world_tree_init();
  view_init();

  setup_accelerators();

  menu_init();
  buffer_size_clear();

  dbus_init();

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

  studio_proxy_set_startstop_callbacks(on_studio_started, on_studio_stopped, on_studio_crashed);

  studio_proxy_set_renamed_callback(on_studio_renamed);
  studio_proxy_set_room_callbacks(room_appeared, room_disappeared, room_changed);

  g_signal_connect(G_OBJECT(g_main_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_quit")), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_view_arrange")), "activate", G_CALLBACK(arrange), NULL);
  g_signal_connect(G_OBJECT(get_gtk_builder_widget("menu_item_help_about")), "activate", G_CALLBACK(show_about), NULL);

  g_signal_connect(G_OBJECT(g_clear_xruns_and_max_dsp_action), "activate", G_CALLBACK(clear_xruns_and_max_dsp), NULL);

  gtk_widget_show(g_main_win);

  //_about_win->set_transient_for(*_main_win);

  gtk_main();

  studio_proxy_uninit();
  control_proxy_uninit();
  dbus_uninit();
  create_room_dialog_uninit();
  uninit_gtk_builder();

  return 0;
}
