/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the GtkBuilder helpers
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
#include "gtk_builder.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

GtkBuilder * g_builder;

bool init_gtk_builder(void)
{
  const char * path;
  struct stat st;
  GError * error_ptr;

  path = "./gui/gladish.ui";
  if (stat(path, &st) == 0)
  {
    goto found;
  }

  path = DATA_DIR "/gladish.ui";
  if (stat(path, &st) == 0)
  {
    goto found;
  }

  log_error("Unable to find the gladish.ui file");
  return false;

found:
  log_info("Loading glade from %s", path);

  g_builder = gtk_builder_new();
  if (g_builder == NULL)
  {
    log_error("gtk_builder_new() failed.");
    return false;
  }

  error_ptr = NULL;
  if (gtk_builder_add_from_file(g_builder, path, &error_ptr) == 0)
  {
    log_error("gtk_builder_add_from_file(\"%s\") failed: %s", path, error_ptr->message);
    g_error_free(error_ptr);
    g_object_unref(g_builder);
    return false;
  }

  return true;
}

void uninit_gtk_builder(void)
{
  g_object_unref(g_builder);
}

GObject * get_gtk_builder_object(const char * name)
{
  GObject * ptr;

  ptr = gtk_builder_get_object(g_builder, name);

  if (ptr == NULL)
  {
    log_error("glade object with id '%s' not found", name);
    ASSERT_NO_PASS;
  }

  return ptr;
}

GtkWidget * get_gtk_builder_widget(const char * name)
{
  return GTK_WIDGET(get_gtk_builder_object(name));
}
