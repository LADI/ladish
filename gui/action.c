/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains GtkAction related code
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
#include "action.h"
#include "gtk_builder.h"
#include "jack.h"
#include "zoom.h"
#include "menu.h"
#include "studio.h"

GtkAction * g_clear_xruns_and_max_dsp_action;
GtkAction * g_zoom_100_action;
GtkAction * g_zoom_fit_action;
GtkAction * g_zoom_in_action;
GtkAction * g_zoom_out_action;

static
gboolean
load_project_accelerator_activated(
  GtkAccelGroup * accel_group,
  GObject * acceleratable,
  guint keyval,
  GdkModifierType modifier)
{
  graph_view_handle view;

  view = get_current_view();
  if (get_studio_state() == STUDIO_STATE_STARTED && view != NULL && is_room_view(view))
  {
    menu_request_load_project();
  }

  return TRUE;
}

static
gboolean
unload_project_accelerator_activated(
  GtkAccelGroup * accel_group,
  GObject * acceleratable,
  guint keyval,
  GdkModifierType modifier)
{
  graph_view_handle view;

  view = get_current_view();
  if (get_studio_state() == STUDIO_STATE_STARTED && view != NULL && is_room_view(view))
  {
    menu_request_unload_project();
  }

  return TRUE;
}

void init_actions_and_accelerators(void)
{
  static GtkActionGroup * action_group_ptr;
  static GtkAccelGroup * accel_group_ptr;

  g_clear_xruns_and_max_dsp_action = GTK_ACTION(get_gtk_builder_object("clear_xruns_and_max_dsp_load_action"));
  g_signal_connect(G_OBJECT(g_clear_xruns_and_max_dsp_action), "activate", G_CALLBACK(clear_xruns_and_max_dsp), NULL);
  gtk_action_set_label(g_clear_xruns_and_max_dsp_action, _("Clear XRuns and Max DSP Load"));
  gtk_action_set_short_label(g_clear_xruns_and_max_dsp_action, _("Clear"));

  g_zoom_100_action = GTK_ACTION(get_gtk_builder_object("zoom_100_action"));
  g_signal_connect(G_OBJECT(g_zoom_100_action), "activate", G_CALLBACK(zoom_100), NULL);
  gtk_action_set_label(g_zoom_100_action, _("Zoom 100%"));
  gtk_action_set_short_label(g_zoom_100_action, _("Zoom 100%"));

  g_zoom_fit_action = GTK_ACTION(get_gtk_builder_object("zoom_fit_action"));
  g_signal_connect(G_OBJECT(g_zoom_fit_action), "activate", G_CALLBACK(zoom_fit), NULL);
  gtk_action_set_label(g_zoom_fit_action, _("Zoom to fit"));
  gtk_action_set_short_label(g_zoom_fit_action, _("Zoom to fit"));

  g_zoom_in_action = GTK_ACTION(get_gtk_builder_object("zoom_in_action"));
  g_signal_connect(G_OBJECT(g_zoom_in_action), "activate", G_CALLBACK(zoom_in), NULL);
  gtk_action_set_label(g_zoom_in_action, _("Zoom in"));
  gtk_action_set_short_label(g_zoom_in_action, _("Zoom in"));

  g_zoom_out_action = GTK_ACTION(get_gtk_builder_object("zoom_out_action"));
  g_signal_connect(G_OBJECT(g_zoom_out_action), "activate", G_CALLBACK(zoom_out), NULL);
  gtk_action_set_label(g_zoom_out_action, _("Zoom out"));
  gtk_action_set_short_label(g_zoom_out_action, _("Zoom out"));

  struct
  {
    GtkAction * action_ptr;
    const char * shortcut;
  } * descriptor_ptr, descriptors [] =
      {
        {g_clear_xruns_and_max_dsp_action, "c"},
        {g_zoom_in_action, "<Control>plus"},
        {g_zoom_out_action, "<Control>minus"},
        {g_zoom_100_action, "<Control>0"},
        {g_zoom_fit_action, "<Control>equal"},
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

  gtk_accel_group_connect(
    accel_group_ptr,
    gdk_keyval_from_name("o"),
    GDK_CONTROL_MASK,
    GTK_ACCEL_VISIBLE,
    g_cclosure_new((GCallback)load_project_accelerator_activated, NULL, NULL));

  gtk_accel_group_connect(
    accel_group_ptr,
    gdk_keyval_from_name("u"),
    GDK_CONTROL_MASK,
    GTK_ACCEL_VISIBLE,
    g_cclosure_new((GCallback)unload_project_accelerator_activated, NULL, NULL));

  gtk_window_add_accel_group(GTK_WINDOW(g_main_win), accel_group_ptr);
}

void enable_action(GtkAction * action)
{
  gtk_action_set_sensitive(action, TRUE);
}

void disable_action(GtkAction * action)
{
  gtk_action_set_sensitive(action, FALSE);
}
