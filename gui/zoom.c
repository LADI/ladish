/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the canvas zoom related code
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

#include "zoom.h"
#include "canvas.h"
#include "graph_view.h"

#define ZOOM_CHANGE 1.1

void zoom_100(void)
{
  canvas_handle canvas;

  log_info("zoom 100%% request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_set_zoom(canvas, 1.0);
  }
}

void zoom_fit(void)
{
  canvas_handle canvas;

  log_info("zoom fit request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_set_zoom_fit(canvas);
  }
}

void zoom_in(void)
{
  canvas_handle canvas;

  log_info("zoom in request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_set_zoom(canvas, canvas_get_zoom(canvas) * ZOOM_CHANGE);
  }
}

void zoom_out(void)
{
  canvas_handle canvas;

  log_info("zoom out request");

  canvas = get_current_canvas();
  if (canvas != NULL)
  {
    canvas_set_zoom(canvas, canvas_get_zoom(canvas) / ZOOM_CHANGE);
  }
}
