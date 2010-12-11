/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the studio handling code
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
#include "studio.h"
#include "menu.h"
#include "../proxies/studio_proxy.h"
#include "statusbar.h"
#include "pixbuf.h"
#include "ask_dialog.h"
#include "jack.h"

/* File names */
#define STATUS_ICON_DOWN      "status_down.png"         /* temporary down during service restart */
#define STATUS_ICON_UNLOADED  "status_unloaded.png"
#define STATUS_ICON_STARTED   "status_started.png"
#define STATUS_ICON_STOPPED   "status_stopped.png"
#define STATUS_ICON_WARNING   "status_warning.png"      /* xruns */
#define STATUS_ICON_ERROR     "status_error.png"        /* bad error */

static unsigned int g_studio_state = STUDIO_STATE_UNKNOWN;
static graph_view_handle g_studio_view = NULL;

unsigned int get_studio_state(void)
{
  return g_studio_state;
}

void set_studio_state(unsigned int state)
{
  g_studio_state = state;
}

bool studio_loaded(void)
{
  return g_studio_view != NULL;
}

void create_studio_view(const char * name)
{
  ASSERT(!studio_loaded());

  if (!create_view(name, SERVICE_NAME, STUDIO_OBJECT_PATH, true, true, false, &g_studio_view))
  {
    log_error("create_view() failed for studio");
  }
}

void destroy_studio_view(void)
{
  ASSERT(studio_loaded());

  destroy_view(g_studio_view);
  g_studio_view = NULL;
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

  switch (get_jack_state())
  {
  case JACK_STATE_NA:
    tooltip = status = _("JACK is sick");
    status_image_path = STATUS_ICON_ERROR;
    break;
  case JACK_STATE_STOPPED:
    status = _("Stopped");
    break;
  case JACK_STATE_STARTED:
    status = _("xruns");
    break;
  default:
    status = "???";
    tooltip = _("Internal error - unknown jack state");
    status_image_path = STATUS_ICON_ERROR;
  }

  buffer = NULL;

  switch (g_studio_state)
  {
  case STUDIO_STATE_NA:
    name = _("ladishd is down");
    status_image_path = STATUS_ICON_DOWN;
    break;
  case STUDIO_STATE_SICK:
  case STUDIO_STATE_UNKNOWN:
    tooltip = name = _("ladishd is sick");
    status_image_path = STATUS_ICON_ERROR;
    break;
  case STUDIO_STATE_UNLOADED:
    name = _("No studio loaded");
    status_image_path = STATUS_ICON_UNLOADED;
    break;
  case STUDIO_STATE_CRASHED:
    status = _("Crashed");
    tooltip = _("Crashed studio, save your work if you can and unload the studio");
    status_image_path = STATUS_ICON_ERROR;
    /* fall through */
  case STUDIO_STATE_STOPPED:
  case STUDIO_STATE_STARTED:
    if (!studio_proxy_get_name(&buffer))
    {
      tooltip = "failed to get studio name";
      log_error("%s", tooltip);
      tooltip = _(tooltip);
      status_image_path = STATUS_ICON_ERROR;
    }
    else
    {
      name = buffer;
      switch (g_studio_state)
      {
      case STUDIO_STATE_STARTED:
        status_image_path = jack_xruns() ? STATUS_ICON_WARNING : STATUS_ICON_STARTED;
        tooltip = _("Studio is started");
        break;
      case STUDIO_STATE_STOPPED:
        status_image_path = STATUS_ICON_STOPPED;
        tooltip = _("Studio is stopped");
        break;
      }
      break;
    }
  default:
    name = "???";
    tooltip = _("Internal error - unknown studio state");
    status_image_path = STATUS_ICON_ERROR;
  }

  set_xrun_progress_bar_text(status);

  set_studio_status_text(name);

  if (status_image_path == NULL || (pixbuf = load_pixbuf(status_image_path)) == NULL)
  {
    gtk_image_set_from_stock(get_status_image(), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  }
  else
  {
    gtk_image_set_from_pixbuf(get_status_image(), pixbuf);
    g_object_unref(pixbuf);
  }

  //gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(g_status_tool_item), tooltip);

  if (get_jack_state() == JACK_STATE_STARTED)
  {
    update_jack_sample_rate();
  }
  else
  {
    clear_sample_rate_text();
    clear_latency_text();
    clear_dsp_load_text();
    clear_xruns_text();
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
  error_message_box(_("JACK crashed or stopped unexpectedly. Save your work, then unload and reload the studio."));
}

static void on_studio_renamed(const char * new_studio_name)
{
  if (studio_loaded())
  {
    set_view_name(g_studio_view, new_studio_name);
    set_studio_status_text(new_studio_name);
  }
}

void menu_request_save_studio(void)
{
  log_info("save studio request");
  if (!studio_proxy_save())
  {
    error_message_box(_("Studio save failed, please inspect logs."));
  }
}

void menu_request_save_as_studio(void)
{
  char * new_name;

  log_info("save as studio request");

  if (name_dialog(_("Save studio as"), _("Studio name"), "", &new_name))
  {
    if (!studio_proxy_save_as(new_name))
    {
      error_message_box(_("Saving of studio failed, please inspect logs."));
    }

    free(new_name);
  }
}

void menu_request_start_studio(void)
{
  log_info("start studio request");
  if (!studio_proxy_start())
  {
    error_message_box(_("Studio start failed, please inspect logs."));
  }
}

void menu_request_stop_studio(void)
{
  log_info("stop studio request");
  if (!studio_proxy_stop())
  {
    error_message_box(_("Studio stop failed, please inspect logs."));
  }
}

void menu_request_unload_studio(void)
{
  log_info("unload studio request");
  if (!studio_proxy_unload())
  {
    error_message_box(_("Studio unload failed, please inspect logs."));
  }
}

void menu_request_rename_studio(void)
{
  char * new_name;

  if (name_dialog(_("Rename studio"), _("Studio name"), get_view_name(g_studio_view), &new_name))
  {
    if (!studio_proxy_rename(new_name))
    {
      error_message_box(_("Studio rename failed, please inspect logs."));
    }

    free(new_name);
  }
}

void set_studio_callbacks(void)
{
  studio_proxy_set_startstop_callbacks(on_studio_started, on_studio_stopped, on_studio_crashed);
  studio_proxy_set_renamed_callback(on_studio_renamed);
}
