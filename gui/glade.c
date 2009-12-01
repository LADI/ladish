/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the glade (gtk_builder) helpers
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glade/glade.h>

GladeXML * g_glade;

bool init_glade(void)
{
  const char * path;
  struct stat st;

  path = "./gui/gui.glade";
  if (stat(path, &st) == 0)
  {
    goto found;
  }

  path = DATA_DIR "/gui.glade";
  if (stat(path, &st) == 0)
  {
    goto found;
  }

  log_error("Unable to find the gui.glade file");
  uninit_glade();
  return false;

found:
  log_info("Loading glade from %s", path);
  g_glade = glade_xml_new(path, NULL, NULL);
  return true;
}

void uninit_glade(void)
{
}

GtkWidget * get_glade_widget(const char * name)
{
  GtkWidget * ptr;

  ptr = GTK_WIDGET(glade_xml_get_widget(g_glade, name));

  if (ptr == NULL)
  {
    log_error("glade object with id '%s' not found", name);
    ASSERT_NO_PASS;
  }

  return ptr;
}
