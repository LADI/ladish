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

void
update_toolbar()
{
  bool started;

  started = _jack->is_started();

  _buffer_size_combo->set_sensitive(started);

  if (started)
  {
    _buffer_size_combo->set_active((int)log2f(_jack->buffer_size()) - 5);
  }
}

void
update_load()
{
  if (!_jack->is_started())
  {
    _main_xrun_progress->set_text("JACK stopped");
    return;
  }

  char tmp_buf[8];
  snprintf(tmp_buf, 8, "%zd", _jack->xruns());

  _main_xrun_progress->set_text(std::string(tmp_buf) + " Dropouts");

  float load = _jack->get_dsp_load();

  load /= 100.0;                // dbus returns it in percents, we use 0..1

  if (load > _max_dsp_load)
  {
    _max_dsp_load = load;
    _main_xrun_progress->set_fraction(load);
  }
}

void
clear_load()
{
  _main_xrun_progress->set_fraction(0.0);
  _jack->reset_xruns();
  _max_dsp_load = 0.0;
}

void
jack_status_changed(
  bool started)
{
  update_toolbar();

  _menu_jack_start->set_sensitive(!started);
  _menu_jack_stop->set_sensitive(started);
  _clear_load_but->set_sensitive(started);
  if (!started)
  {
    _main_xrun_progress->set_fraction(0.0);
  }
}

void
buffer_size_changed()
{
  const int selected = _buffer_size_combo->get_active_row_number();

  if (selected == -1)
  {
    update_toolbar();
  }
  else
  {
    uint32_t buffer_size = 1 << (selected+5);
  
    // this check is temporal workaround for jack bug
    // we skip setting buffer size if it same as acutal one
    // proper place for such check is in jack
    if (_jack->buffer_size() != buffer_size)
    {
      if (!_jack->set_buffer_size(buffer_size))
      {
        update_toolbar(); // reset combo box to actual value
      }
    }
  }
}

#endif

GtkWidget * g_main_win;
graph_view_handle g_jack_view = NULL;
graph_view_handle g_studio_view = NULL;

void control_proxy_on_studio_appeared(void)
{
  if (!create_view("Studio", SERVICE_NAME, STUDIO_OBJECT_PATH, false, &g_studio_view))
  {
    lash_error("create_view() failed for studio");
    return;
  }
}

void control_proxy_on_studio_disappeared(void)
{
  if (g_studio_view != NULL)
  {
    destroy_view(g_studio_view);
    g_jack_view = NULL;
  }
}

void jack_started(void)
{
  lash_info("JACK started");
}

void jack_stopped(void)
{
  lash_info("JACK stopped");
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

  //gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);

  g_signal_connect(G_OBJECT(g_main_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_glade_widget("menu_file_quit")), "activate", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show(g_main_win);

  //_about_win->set_transient_for(*_main_win);

  gtk_main();

  control_proxy_uninit();
  uninit_glade();

  return 0;
}
