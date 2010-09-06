/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that implements pixel buffer helper functionality
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
#include "../catdup.h"

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

GdkPixbuf * load_pixbuf(const char * filename)
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
