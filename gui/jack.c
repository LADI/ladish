/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *
 **************************************************************************
 * This file contains the JACK related functionality
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

#include <math.h>

#include "graph_view.h"
#include "studio.h"
#include "menu.h"
#include "statusbar.h"
#include "action.h"
#include "../proxies/jack_proxy.h"
#include "gtk_builder.h"

/* JACK states */
#define JACK_STATE_NA         0
#define JACK_STATE_STOPPED    1
#define JACK_STATE_STARTED    2

unsigned int g_jack_state = JACK_STATE_NA;
static uint32_t g_xruns;
static double g_jack_max_dsp_load = 0.0;
static GtkWidget * g_xrun_progress_bar;
static uint32_t g_sample_rate;
static bool g_jack_view_enabled = false;
static graph_view_handle g_jack_view = NULL;
static guint g_jack_poll_source_tag;

static void update_raw_jack_visibility(void)
{
  /* if there is no jack view and its display is enabled and jack is avaialable, create the raw jack view */
  if (g_jack_view == NULL && g_jack_view_enabled && g_jack_state != JACK_STATE_NA)
  {
    if (!create_view(_("Raw JACK"), JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, false, false, true, &g_jack_view))
    {
      log_error("create_view() failed for jack");
      return;
    }
  }

  /* if there is jack view and its display is disabled or it is enabled byt jack is not avaialable, destroy the raw jack view */
  if (g_jack_view != NULL && (!g_jack_view_enabled || g_jack_state == JACK_STATE_NA))
  {
    destroy_view(g_jack_view);
    g_jack_view = NULL;
  }
}

void buffer_size_clear(void)
{
  menu_set_jack_latency_items_sensivity(false);
  clear_latency_text();
}

static void buffer_size_set(uint32_t size, bool force)
{
  char buf[100];
  static uint32_t last_buffer_size;

  if (!menu_set_jack_latency(size, force))
  {
    buffer_size_clear();
    return;
  }

  if (size != last_buffer_size)
  {
    log_info("JACK latency changed: %"PRIu32" samples", size);

    snprintf(buf, sizeof(buf), _("%4.1f ms (%"PRIu32")"), (float)size / (float)g_sample_rate * 1000.0f, size);
    set_latency_text(buf);
  }
  last_buffer_size = size;
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
    snprintf(tmp_buf, sizeof(tmp_buf), _("%" PRIu32 " dropouts"), xruns);
    set_xruns_text(tmp_buf);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), tmp_buf);
  }
  else
  {
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), _("error"));
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
    set_xruns_text("?");
  }

  if (jack_proxy_get_dsp_load(&load))
  {
    if (load > g_jack_max_dsp_load)
    {
      g_jack_max_dsp_load = load;
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), load / 100.0);
    }

    snprintf(tmp_buf, sizeof(tmp_buf), _("DSP: %5.1f%% (%5.1f%%)"), (float)load, (float)g_jack_max_dsp_load);
    set_dsp_load_text(tmp_buf);
  }
  else
  {
    set_xruns_text("?");
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

static gboolean poll_jack(gpointer data)
{
  update_load();
  update_buffer_size(false);

  return TRUE;
}

static void jack_appeared(void)
{
  log_info("JACK appeared");

  g_jack_state = JACK_STATE_STOPPED;
  studio_state_changed(NULL);
  update_raw_jack_visibility();
}

static void jack_started(void)
{
  log_info("JACK started");

  g_jack_state = JACK_STATE_STARTED;
  studio_state_changed(NULL);

  menu_set_jack_latency_items_sensivity(true);
  update_buffer_size(true);
  enable_action(g_clear_xruns_and_max_dsp_action);

  g_jack_poll_source_tag = g_timeout_add(100, poll_jack, NULL);
}

static void jack_stopped(void)
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
  disable_action(g_clear_xruns_and_max_dsp_action);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
}

static void jack_disappeared(void)
{
  log_info("JACK disappeared");

  jack_stopped();

  g_jack_state = JACK_STATE_NA;
  studio_state_changed(NULL);
  update_raw_jack_visibility();
}

void clear_xruns_and_max_dsp(void)
{
  log_info("clearing xruns and max dsp load");
  jack_proxy_reset_xruns();
  g_jack_max_dsp_load = 0.0;
}

void menu_request_jack_latency_change(uint32_t buffer_size)
{
  log_info("JACK latency change request: %"PRIu32" samples", buffer_size);

  if (!jack_proxy_set_buffer_size(buffer_size))
  {
    log_error("cannot set JACK buffer size");
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
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), _("<b><big>Error executing ladiconf.\nAre LADI Tools installed?</big></b>"));
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), _("%s"), error_ptr->message);
    gtk_widget_show(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    g_error_free(error_ptr);
  }
}

void menu_request_toggle_raw_jack(bool visible)
{
  //log_info("toogle raw jack visibility -> %s", visible ? "visible" : "invisible");
  g_jack_view_enabled = visible;
  update_raw_jack_visibility();
}

void init_jack_widgets(void)
{
  g_xrun_progress_bar = get_gtk_builder_widget("xrun_progress_bar");
}

bool init_jack(void)
{
  return jack_proxy_init(jack_started, jack_stopped, jack_appeared, jack_disappeared);
}

void uninit_jack(void)
{
  jack_proxy_uninit();
}

bool jack_xruns(void)
{
  return g_xruns != 0;
}

void set_xrun_progress_bar_text(const char * text)
{
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_xrun_progress_bar), text);

  /* workaround a bug in GtkProgressBar. it needs fraction change in order to redraw the changed text
   * GtkProgressBar tracks the dirty state and it is checked before painting, so sending redraw request
   * through gtk_widget_queue_draw() will not help because expose handler will ignore the redraw request */
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 1.0);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_xrun_progress_bar), 0.0);
}

void update_jack_sample_rate(void)
{
  if (jack_proxy_sample_rate(&g_sample_rate))
  {
    char buf[100];

    if (fmod(g_sample_rate, 1000.0) != 0.0)
    {
      snprintf(buf, sizeof(buf), _("%.1f kHz"), (float)g_sample_rate / 1000.0f);
    }
    else
    {
      snprintf(buf, sizeof(buf), _("%u kHz"), g_sample_rate / 1000);
    }

    set_sample_rate_text(buf);
  }
  else
  {
    clear_sample_rate_text();
  }
}

unsigned int get_jack_state(void)
{
  return g_jack_state;
}
