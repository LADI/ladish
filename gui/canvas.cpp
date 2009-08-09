/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implements the canvas functionality through flowcanvas
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

#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Port.hpp>
#include <flowcanvas/Module.hpp>

#include "canvas.h"

bool
canvas_create(
  int width,
  int height,
  canvas_handle * canvas_handle_ptr)
{
  return false;
}

bool
canvas_destroy(
  canvas_handle canvas)
{
  return false;
}

void
canvas_clear(
  canvas_handle canvas)
{
}

void
canvas_set_zoom(
  canvas_handle canvas,
  double pix_per_unit)
{
}

void
canvas_arrange(
  canvas_handle canvas)
{
}

bool
canvas_create_module(
  canvas_handle canvas,
  const char * name,
  double x,
  double y,
  bool show_title,
  bool show_port_labels,
  canvas_module_handle * module_handle_ptr)
{
  return false;
}

bool
canvas_destroy_module(
  canvas_handle canvas,
  canvas_module_handle module)
{
  return false;
}

bool
canvas_create_port(
  canvas_handle canvas,
  canvas_module_handle module,
  const char * name,
  bool is_input,
  int color)
{
  return false;
}

bool
canvas_destroy_port(
  canvas_handle canvas,
  canvas_port_handle port)
{
  return false;
}

bool
canvas_add_connection(
  canvas_handle canvas,
  canvas_port_handle port1,
  canvas_port_handle port2,
  uint32_t color)
{
  return false;
}

bool
canvas_remove_connection(
  canvas_handle canvas,
  canvas_port_handle port1,
  canvas_port_handle port2)
{
  return false;
}

bool
canvas_enum_modules(
  canvas_handle canvas,
  void * callback_context,
  bool (* callback)(void * context, canvas_module_handle module))
{
  return false;
}

bool
canvas_enum_module_ports(
  canvas_handle canvas,
  canvas_module_handle module,
  void * callback_context,
  bool (* callback)(void * context, canvas_port_handle port))
{
  return false;
}
