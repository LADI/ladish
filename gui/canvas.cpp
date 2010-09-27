/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
    void (* disconnect_request)(void * port1_context, void * port2_context),
    void (* module_location_changed)(void * module_context, double x, double y),
    void (* fill_canvas_menu)(GtkMenu * menu),
    void (* fill_module_menu)(GtkMenu * menu, void * module_context),
    void (* fill_port_menu)(GtkMenu * menu, void * port_context))
    : FlowCanvas::Canvas(width, height)
    , m_connect_request(connect_request)
    , m_disconnect_request(disconnect_request)
    , m_module_location_changed(module_location_changed)
    , m_fill_canvas_menu(fill_canvas_menu)
    , m_fill_module_menu(fill_module_menu)
    , m_fill_port_menu(fill_port_menu)
  {}

  virtual ~canvas_cls() {}

  virtual void on_realize()
  {
    log_info("canvas_cls::on_realize");
    FlowCanvas::Canvas::on_realize();
    scroll_to_center();
  }

  virtual void on_size_allocate(Gtk::Allocation& allocation)
  {
    //log_info("canvas_cls::on_size_allocate");
    FlowCanvas::Canvas::on_size_allocate(allocation);
    if (is_realized())
    {
      //log_info("... realized");
      scroll_to_center();
    }
  }

  virtual bool canvas_event(GdkEvent * event);

  virtual void connect(boost::shared_ptr<FlowCanvas::Connectable> port1, boost::shared_ptr<FlowCanvas::Connectable> port2);
  virtual void disconnect(boost::shared_ptr<FlowCanvas::Connectable> port1, boost::shared_ptr<FlowCanvas::Connectable> port2);

  void (* m_connect_request)(void * port1_context, void * port2_context);
  void (* m_disconnect_request)(void * port1_context, void * port2_context);
  void (* m_module_location_changed)(void * module_context, double x, double y);
  void (* m_fill_canvas_menu)(GtkMenu * menu);
  void (* m_fill_module_menu)(GtkMenu * menu, void * module_context);
  void (* m_fill_port_menu)(GtkMenu * menu, void * port_context);
};

class module_cls: public FlowCanvas::Module
{
public:
  module_cls(
    boost::shared_ptr<FlowCanvas::Canvas> canvas,
    const std::string& name,
    double x,
    double y,
    bool show_title,
    bool show_port_labels,
    void * module_context)
    : FlowCanvas::Module(canvas, name, x, y, show_title, show_port_labels)
    , m_context(module_context)
  {}

  virtual ~module_cls() {}

  virtual void store_location()
  {
    boost::dynamic_pointer_cast<canvas_cls>(canvas().lock())->m_module_location_changed(m_context, property_x(), property_y());
  }

  void create_menu()
  {
    _menu = new Gtk::Menu();
    _menu->items().push_back(Gtk::Menu_Helpers::MenuElem("Disconnect All", sigc::mem_fun(this, &module_cls::menu_disconnect_all)));
    void (* fill_module_menu)(GtkMenu * menu, void * module_context) = boost::dynamic_pointer_cast<canvas_cls>(canvas().lock())->m_fill_module_menu;
    if (fill_module_menu != NULL)
    {
      fill_module_menu(_menu->gobj(), m_context);
    }
  }

  void menu_disconnect_all()
  {
    for (FlowCanvas::PortVector::iterator p = _ports.begin(); p != _ports.end(); p++)
    {
      (*p)->disconnect_all();
    }
  }

  void * m_context;
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
  {}

  virtual ~port_cls() {}

  void * context;
};

bool
canvas_init(
  void)
{
  Gnome::Canvas::init();
  return true;
}

bool
canvas_create(
  double width,
  double height,
  void (* connect_request)(void * port1_context, void * port2_context),
  void (* disconnect_request)(void * port1_context, void * port2_context),
  void (* module_location_changed)(void * module_context, double x, double y),
  void (* fill_canvas_menu)(GtkMenu * menu),
  void (* fill_module_menu)(GtkMenu * menu, void * module_context),
  void (* fill_port_menu)(GtkMenu * menu, void * port_context),
  canvas_handle * canvas_handle_ptr)
{
  boost::shared_ptr<canvas_cls> * canvas;

  canvas = new boost::shared_ptr<canvas_cls>(new canvas_cls(width,
                                                            height,
                                                            connect_request,
                                                            disconnect_request,
                                                            module_location_changed,
                                                            fill_canvas_menu,
                                                            fill_module_menu,
                                                            fill_port_menu));

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
      ASSERT(port != NULL);
      module->remove_port(port);
      port->hide();
    }

    ASSERT(module->ports().empty());
    canvas_ptr->get()->remove_item(module);
  }
}

void
canvas_get_size(
  canvas_handle canvas,
  double * width_ptr,
  double * height_ptr)
{
  *width_ptr = canvas_ptr->get()->width();
  *height_ptr = canvas_ptr->get()->height();
}

void
canvas_scroll_to_center(
  canvas_handle canvas)
{
  if (canvas_ptr->get()->is_realized())
  {
    //log_info("realized");
    canvas_ptr->get()->scroll_to_center();
  }
  else
  {
    //log_info("NOT realized");
  }
}

double
canvas_get_zoom(
  canvas_handle canvas)
{
  return canvas_ptr->get()->get_zoom();
}

void
canvas_set_zoom(
  canvas_handle canvas,
  double pix_per_unit)
{
  canvas_ptr->get()->set_zoom(pix_per_unit);
}

void
canvas_set_zoom_fit(
  canvas_handle canvas)
{
  canvas_ptr->get()->zoom_full();
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
  void * module_context,
  canvas_module_handle * module_handle_ptr)
{
  boost::shared_ptr<FlowCanvas::Module> * module;

  module = new boost::shared_ptr<FlowCanvas::Module>(new module_cls(*canvas_ptr, name, x, y, show_title, show_port_labels, module_context));
  canvas_ptr->get()->add_item(*module);
  module->get()->resize();

  *module_handle_ptr = (canvas_module_handle)module;

  return true;
}

#define module_ptr ((boost::shared_ptr<FlowCanvas::Module> *)module)

void
canvas_set_module_name(
  canvas_module_handle module,
  const char * name)
{
  module_ptr->get()->set_name(name);
}

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

void
canvas_set_port_name(
  canvas_port_handle port,
  const char * name)
{
  port_ptr->get()->set_name(name);
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

bool canvas_cls::canvas_event(GdkEvent * event)
{
	assert(event);

	if (m_fill_canvas_menu != NULL && event->type == GDK_BUTTON_PRESS && event->button.button == 3)
  {
    Gtk::Menu * menu_ptr;
    menu_ptr = new Gtk::Menu();
    m_fill_canvas_menu(menu_ptr->gobj());
    menu_ptr->show_all();
    menu_ptr->popup(event->button.button, event->button.time);
    return true;
  }

	return Canvas::canvas_event(event);
}

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
