/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the statusbar helpers
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

#include "statusbar.h"
#include "gtk_builder.h"

static GtkStatusbar * g_statusbar;
static GtkWidget * g_studio_status_label;
static GtkWidget * g_status_image;
static GtkWidget * g_sample_rate_label;
static GtkWidget * g_latency_label;
static GtkWidget * g_xruns_label;
static GtkWidget * g_dsp_load_label;

static void pack_statusbar_widget(GtkWidget * widget)
{
  static int index;

  gtk_widget_show(widget);
  gtk_box_pack_start(GTK_BOX(g_statusbar), widget, FALSE, TRUE, 10);
  gtk_box_reorder_child(GTK_BOX(g_statusbar), widget, index++);
}

static void init_statusbar_label_widget(const char * id, GtkWidget ** widget_ptr_ptr)
{
  *widget_ptr_ptr = gtk_label_new(id);
  pack_statusbar_widget(*widget_ptr_ptr);
}

void init_statusbar(void)
{
  g_statusbar = GTK_STATUSBAR(get_gtk_builder_widget("statusbar"));

  init_statusbar_label_widget("studioname", &g_studio_status_label);

  g_status_image = gtk_image_new();
  pack_statusbar_widget(g_status_image);

  init_statusbar_label_widget("srate", &g_sample_rate_label);
  init_statusbar_label_widget("latency", &g_latency_label);
  init_statusbar_label_widget("xruns", &g_xruns_label);
  init_statusbar_label_widget("load", &g_dsp_load_label);
}

void set_studio_status_text(const char * text)
{
  gtk_label_set_text(GTK_LABEL(g_studio_status_label), text);
}

GtkImage * get_status_image(void)
{
  return GTK_IMAGE(g_status_image);
}

void set_latency_text(const char * text)
{
  gtk_label_set_text(GTK_LABEL(g_latency_label), text);
}

void clear_latency_text(void)
{
  gtk_label_set_text(GTK_LABEL(g_latency_label), "");
}

void set_dsp_load_text(const char * text)
{
  gtk_label_set_text(GTK_LABEL(g_dsp_load_label), text);
}

void clear_dsp_load_text(void)
{
  gtk_label_set_text(GTK_LABEL(g_dsp_load_label), "");
}

void set_xruns_text(const char * text)
{
  gtk_label_set_text(GTK_LABEL(g_xruns_label), text);
}

void clear_xruns_text(void)
{
  gtk_label_set_text(GTK_LABEL(g_xruns_label), "");
}

void set_sample_rate_text(const char * text)
{
  gtk_label_set_text(GTK_LABEL(g_sample_rate_label), text);
}

void clear_sample_rate_text(void)
{
  gtk_label_set_text(GTK_LABEL(g_sample_rate_label), "");
}
