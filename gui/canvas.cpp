/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

class canvas_cls: public FlowCanvas::Canvas
{
public:
  canvas_cls(
    double width,
    double height,
    void (* connect_request)(void * port1_context, void * port2_context),
    void (* disconnect_request)(void * port1_context, void * port2_context))
    : FlowCanvas::Canvas(width, height)
    , m_connect_request(connect_request)
    , m_disconnect_request(disconnect_request)
  {};

  virtual ~canvas_cls() {};
  virtual void connect(boost::shared_ptr<FlowCanvas::Connectable> port1, boost::shared_ptr<FlowCanvas::Connectable> port2);
  virtual void disconnect(boost::shared_ptr<FlowCanvas::Connectable> port1, boost::shared_ptr<FlowCanvas::Connectable> port2);

  void (* m_connect_request)(void * port1_context, void * port2_context);
  void (* m_disconnect_request)(void * port1_context, void * port2_context);
};

class port_cls: public FlowCanvas::Port
{
public:
  port_cls(
    boost::shared_ptr<FlowCanvas::Module> module,
    const std::string& name,
    bool is_input,
    uint32_t color,
    void * port_context)
    : FlowCanvas::Port(module, name, is_input, color)
    , context(port_context)
  {};

  virtual ~port_cls() {};

  void * context;
};

bool
canvas_create(
  double width,
  double height,
  void (* connect_request)(void * port1_context, void * port2_context),
  void (* disconnect_request)(void * port1_context, void * port2_context),
  canvas_handle * canvas_handle_ptr)
{
  boost::shared_ptr<canvas_cls> * canvas;

  canvas = new boost::shared_ptr<canvas_cls>(new canvas_cls(width, height, connect_request, disconnect_request));

  *canvas_handle_ptr = (canvas_handle)canvas;

  return true;
}

#define canvas_ptr ((boost::shared_ptr<canvas_cls> *)canvas)

GtkWidget *
canvas_get_widget(
  canvas_handle canvas)
{
  return ((Gtk::Widget *)canvas_ptr->get())->gobj();
}

void
canvas_destroy(
  canvas_handle canvas)
{
  delete canvas_ptr;
}

void
canvas_clear(
  canvas_handle canvas)
{
  FlowCanvas::ItemList modules = canvas_ptr->get()->items(); // copy
  for (FlowCanvas::ItemList::iterator m = modules.begin(); m != modules.end(); ++m)
  {
    boost::shared_ptr<FlowCanvas::Module> module = boost::dynamic_pointer_cast<FlowCanvas::Module>(*m);
    if (!module)
      continue;

    FlowCanvas::PortVector ports = module->ports(); // copy
    for (FlowCanvas::PortVector::iterator p = ports.begin(); p != ports.end(); ++p)
    {
      boost::shared_ptr<FlowCanvas::Port> port = boost::dynamic_pointer_cast<FlowCanvas::Port>(*p);
      assert(port != NULL);
      module->remove_port(port);
      port->hide();
    }

    assert(module->ports().empty());
    canvas_ptr->get()->remove_item(module);
  }
}

void
canvas_set_zoom(
  canvas_handle canvas,
  double pix_per_unit)
{
  canvas_ptr->get()->set_zoom(pix_per_unit);
}

void
canvas_arrange(
  canvas_handle canvas)
{
  Glib::RefPtr<Gdk::Window> win = canvas_ptr->get()->get_window();
  if (win)
  {
    canvas_ptr->get()->arrange();
  }
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
  boost::shared_ptr<FlowCanvas::Module> * module;

  module = new boost::shared_ptr<FlowCanvas::Module>(new FlowCanvas::Module(*canvas_ptr, name, x, y, show_title, show_port_labels));
  canvas_ptr->get()->add_item(*module);
  module->get()->resize();

  *module_handle_ptr = (canvas_module_handle)module;

  return true;
}

#define module_ptr ((boost::shared_ptr<FlowCanvas::Module> *)module)

bool
canvas_destroy_module(
  canvas_handle canvas,
  canvas_module_handle module)
{
  canvas_ptr->get()->remove_item(*module_ptr);
  delete module_ptr;
  return true;
}

bool
canvas_create_port(
  canvas_handle canvas,
  canvas_module_handle module,
  const char * name,
  bool is_input,
  int color,
  void * port_context,
  canvas_port_handle * port_handle_ptr)
{
  boost::shared_ptr<port_cls> * port;

  port = new boost::shared_ptr<port_cls>(new port_cls(*module_ptr, name, is_input, color, port_context));

  module_ptr->get()->add_port(*port);
  module_ptr->get()->resize();

  *port_handle_ptr = (canvas_port_handle)port;

  return true;
}

#undef module_ptr
#define port_ptr ((boost::shared_ptr<port_cls> *)port)

bool
canvas_destroy_port(
  canvas_handle canvas,
  canvas_port_handle port)
{
  boost::shared_ptr<FlowCanvas::Module> module = port_ptr->get()->module().lock();
  module->remove_port(*port_ptr);
  delete port_ptr;
  module->resize();
  return true;
}

int
canvas_get_port_color(
  canvas_port_handle port)
{
  return port_ptr->get()->color();
}

#undef port_ptr
#define port1_ptr ((boost::shared_ptr<port_cls> *)port1)
#define port2_ptr ((boost::shared_ptr<port_cls> *)port2)

bool
canvas_add_connection(
  canvas_handle canvas,
  canvas_port_handle port1,
  canvas_port_handle port2,
  uint32_t color)
{
  canvas_ptr->get()->add_connection(*port1_ptr, *port2_ptr, color);
  return true;
}

bool
canvas_remove_connection(
  canvas_handle canvas,
  canvas_port_handle port1,
  canvas_port_handle port2)
{
  canvas_ptr->get()->remove_connection(*port1_ptr, *port2_ptr);
  return true;
}

#undef port1_ptr
#undef port2_ptr

void
canvas_cls::connect(
  boost::shared_ptr<FlowCanvas::Connectable> c1,
  boost::shared_ptr<FlowCanvas::Connectable> c2)
{
  if (m_connect_request != NULL)
  {
    boost::shared_ptr<port_cls> port1 = boost::dynamic_pointer_cast<port_cls>(c1);
    boost::shared_ptr<port_cls> port2 = boost::dynamic_pointer_cast<port_cls>(c2);
    m_connect_request(port1->context, port2->context);
  }
}

void
canvas_cls::disconnect(
  boost::shared_ptr<FlowCanvas::Connectable> c1,
  boost::shared_ptr<FlowCanvas::Connectable> c2)
{
  if (m_disconnect_request != NULL)
  {
    boost::shared_ptr<port_cls> port1 = boost::dynamic_pointer_cast<port_cls>(c1);
    boost::shared_ptr<port_cls> port2 = boost::dynamic_pointer_cast<port_cls>(c2);
    m_disconnect_request(port1->context, port2->context);
  }
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
