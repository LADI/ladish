/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *
 **************************************************************************
 * This file contains the code that implements main() and other top-level functionality
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
#include "canvas.h"
#include "graph_canvas.h"
#include "../jack_proxy.h"
#include "dbus_helpers.h"
#include "control_proxy.h"
#include "../dbus_constants.h"
#include "world_tree.h"

#if 0
class Patchage {
public:
  Patchage();
  ~Patchage();

  Gtk::Window* window() { return _main_win.get(); }
  
  void quit() { _main_win->hide(); }

  void        refresh();

  void clear_load();
  void update_state();
  
  void set_studio_availability(bool available);
#if 0
  void set_a2j_status(unsigned int status);

  void load_project_ask();
  void load_project(const std::string& project_name);
  void save_all_projects();
  void save_project(const std::string& project_name);
  void close_project(const std::string& project_name);
  void close_all_projects();
#endif

protected:
  void connect_widgets();

  void on_arrange();
  void on_help_about();
  void on_quit();
  void on_show_projects();
  void on_view_toolbar();
  bool on_scroll(GdkEventScroll* ev);

  void zoom(double z);
  bool idle_callback();
  void update_load();
  void update_toolbar();

  void jack_status_changed(bool started);

  void buffer_size_changed();

#if 0
  void
  get_port_jack_names(
    boost::shared_ptr<PatchagePort> port,
    std::string& jack_client_name,
    std::string& jack_port_name);
  
  boost::shared_ptr<PatchagePort>
  lookup_port(
    const char * jack_client_name,
    const char * jack_port_name);
#endif

  //a2j_proxy * _a2j;
  //session * _session;
  //lash_proxy * _lash;
  //project_list * _project_list;

  Gtk::Main* _gtk_main;

  float _max_dsp_load;
  
  Widget<Gtk::AboutDialog>    _about_win;
  Widget<Gtk::ComboBox>       _buffer_size_combo;
  Widget<Gtk::ToolButton>     _clear_load_but;
  Widget<Gtk::ScrolledWindow> _main_scrolledwin;
  Widget<Gtk::Window>         _main_win;
  Widget<Gtk::ProgressBar>    _main_xrun_progress;
  Widget<Gtk::Label>          _main_a2j_status_label;
  Widget<Gtk::MenuItem>       _menu_file_quit;
  Widget<Gtk::MenuItem>       _menu_help_about;
  Widget<Gtk::MenuItem>       _menu_jack_start;
  Widget<Gtk::MenuItem>       _menu_jack_stop;
  Widget<Gtk::MenuItem>       _menu_a2j_start;
  Widget<Gtk::MenuItem>       _menu_a2j_stop;
  Widget<Gtk::MenuItem>       _menu_load_project;
  Widget<Gtk::MenuItem>       _menu_save_all_projects;
  Widget<Gtk::MenuItem>       _menu_close_all_projects;
  Widget<Gtk::MenuItem>       _menu_store_positions;
  Widget<Gtk::MenuItem>       _menu_view_arrange;
  Widget<Gtk::CheckMenuItem>  _menu_view_projects;
  Widget<Gtk::MenuItem>       _menu_view_refresh;
  Widget<Gtk::CheckMenuItem>  _menu_view_toolbar;
  Widget<Gtk::Viewport>       _project_list_viewport;
  Widget<Gtk::Label>          _sample_rate_label;
  Widget<Gtk::Toolbar>        _toolbar;
  Widget<Gtk::ToolButton>     _zoom_full_but;
  Widget<Gtk::ToolButton>     _zoom_normal_but;
};

#include "globals.hpp"

#include "common.h"
#include "Widget.hpp"
#include <gtkmm.h>

#include "common.h"

#include <string.h>
#include <cmath>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <gtk/gtkwindow.h>
#include <boost/format.hpp>

#include "project_list.hpp"
#include "session.hpp"
#include "globals.hpp"
//#include "load_projects_dialog.hpp"
#include "graph_canvas.h"
#include "../jack_proxy.h"

Patchage * g_app;

#define LOG_TO_STD
//#define LOG_TO_STATUS

/* Gtk helpers (resize combo boxes) */

static void
gtkmm_get_ink_pixel_size (Glib::RefPtr<Pango::Layout> layout,
             int& width,
             int& height)
{
  Pango::Rectangle ink_rect = layout->get_ink_extents ();
  
  width = (ink_rect.get_width() + PANGO_SCALE / 2) / PANGO_SCALE;
  height = (ink_rect.get_height() + PANGO_SCALE / 2) / PANGO_SCALE;
}

static void
gtkmm_set_width_for_given_text (Gtk::Widget &w, const gchar *text,
               gint hpadding/*, gint vpadding*/)
  
{
  int old_width, old_height;
  w.get_size_request(old_width, old_height);

  int width, height;
  w.ensure_style ();
  
  gtkmm_get_ink_pixel_size (w.create_pango_layout (text), width, height);
  w.set_size_request(width + hpadding, old_height);//height + vpadding);
}

/* end Gtk helpers */


#define INIT_WIDGET(x) x(g_xml, ((const char*)#x) + 1)

Patchage::Patchage()
  : _max_dsp_load(0.0)
#if 0
  , INIT_WIDGET(_about_win)
  , INIT_WIDGET(_buffer_size_combo)
  , INIT_WIDGET(_clear_load_but)
  , INIT_WIDGET(_main_scrolledwin)
  , INIT_WIDGET(_main_win)
  , INIT_WIDGET(_main_xrun_progress)
  , INIT_WIDGET(_main_a2j_status_label)
  , INIT_WIDGET(_menu_file_quit)
  , INIT_WIDGET(_menu_help_about)
  , INIT_WIDGET(_menu_jack_start)
  , INIT_WIDGET(_menu_jack_stop)
  , INIT_WIDGET(_menu_a2j_start)
  , INIT_WIDGET(_menu_a2j_stop)
  , INIT_WIDGET(_menu_load_project)
  , INIT_WIDGET(_menu_save_all_projects)
  , INIT_WIDGET(_menu_close_all_projects)
  , INIT_WIDGET(_menu_store_positions)
  , INIT_WIDGET(_menu_view_arrange)
  , INIT_WIDGET(_menu_view_projects)
  , INIT_WIDGET(_menu_view_refresh)
  , INIT_WIDGET(_menu_view_toolbar)
  , INIT_WIDGET(_project_list_viewport)
  , INIT_WIDGET(_sample_rate_label)
  , INIT_WIDGET(_toolbar)
  , INIT_WIDGET(_zoom_full_but)
  , INIT_WIDGET(_zoom_normal_but)
#endif
{
  g_app = this;

  patchage_dbus_init();

  graph_create(JACKDBUS_SERVICE, JACKDBUS_OBJECT, &g_jack_graph);
  graph_canvas_create(1600 * 2, 1200 * 2, &g_jack_graph_canvas);
  graph_canvas_attach(g_jack_graph_canvas, g_jack_graph);
  graph_activate(g_jack_graph);

  Glib::set_application_name("Patchage");
  _about_win->property_program_name() = "Patchage";
  _about_win->property_logo_icon_name() = "gladish";
  gtk_window_set_default_icon_name("gladish");
  
  gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);

  _main_scrolledwin->add(*Glib::wrap(canvas_get_widget(graph_canvas_get_canvas(g_jack_graph_canvas))));

//   _canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
//                      static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

  _main_scrolledwin->property_hadjustment().get_value()->set_step_increment(10);
  _main_scrolledwin->property_vadjustment().get_value()->set_step_increment(10);

  _main_scrolledwin->signal_scroll_event().connect(
      sigc::mem_fun(this, &Patchage::on_scroll));

  _buffer_size_combo->signal_changed().connect(
      sigc::mem_fun(this, &Patchage::buffer_size_changed));
  _clear_load_but->signal_clicked().connect(
      sigc::mem_fun(this, &Patchage::clear_load));
  _zoom_normal_but->signal_clicked().connect(sigc::bind(
      sigc::mem_fun(this, &Patchage::zoom), 1.0));
//   _zoom_full_but->signal_clicked().connect(sigc::mem_fun(_canvas.get(), &PatchageCanvas::zoom_full));

//  _menu_load_project->signal_activate().connect(sigc::mem_fun(this, &Patchage::load_project_ask));
//  _menu_save_all_projects->signal_activate().connect(sigc::mem_fun(this, &Patchage::save_all_projects));
//  _menu_close_all_projects->signal_activate().connect(sigc::mem_fun(this, &Patchage::close_all_projects));

  _menu_file_quit->signal_activate().connect(
      sigc::mem_fun(this, &Patchage::on_quit));
  _menu_view_refresh->signal_activate().connect(
      sigc::mem_fun(this, &Patchage::refresh));
  _menu_view_arrange->signal_activate().connect(
      sigc::mem_fun(this, &Patchage::on_arrange));
  _menu_view_toolbar->signal_activate().connect(
      sigc::mem_fun(this, &Patchage::on_view_toolbar));
  _menu_view_projects->signal_toggled().connect(
      sigc::mem_fun(this, &Patchage::on_show_projects));
  _menu_help_about->signal_activate().connect(
      sigc::mem_fun(this, &Patchage::on_help_about));

  Glib::wrap(canvas_get_widget(graph_canvas_get_canvas(g_jack_graph_canvas)))->show();
  _main_win->present();

  _about_win->set_transient_for(*_main_win);

//  _a2j = new a2j_proxy;

  //info_msg(str(boost::format("a2j jack client name is '%s'") % _a2j->get_jack_client_name()));

//  _session = new session();

//  _project_list = new project_list(this, _session);
//  _project_list_viewport->hide();

//  _lash = new lash_proxy(_session);

  //_menu_jack_start->signal_activate().connect(sigc::mem_fun(_jack, &jack_proxy::start_server));
  //_menu_jack_stop->signal_activate().connect(sigc::mem_fun(_jack, &jack_proxy::stop_server));

//  _menu_a2j_start->signal_activate().connect(sigc::mem_fun(_a2j, &a2j_proxy::start_bridge));
//  _menu_a2j_stop->signal_activate().connect(sigc::mem_fun(_a2j, &a2j_proxy::stop_bridge));

  //jack_status_changed(_jack->is_started());

  connect_widgets();
  update_state();

//   _canvas->grab_focus();

  // Idle callback, check if we need to refresh
  Glib::signal_timeout().connect(
      sigc::mem_fun(this, &Patchage::idle_callback), 100);
}

Patchage::~Patchage() 
{
  //delete _lash;
  //delete _project_list;
  //delete _session;
  //delete _a2j;

  _about_win.destroy();
  //_main_win.destroy();

  patchage_dbus_uninit();
}

bool
Patchage::idle_callback() 
{
  update_load();

  return true;
}

void
Patchage::update_toolbar()
{
#if 0
  bool started;

  started = _jack->is_started();

  _buffer_size_combo->set_sensitive(started);

  if (started)
  {
    _buffer_size_combo->set_active((int)log2f(_jack->buffer_size()) - 5);
  }
#endif
}

void
Patchage::update_load()
{
#if 0
  if (!_jack->is_started())
  {
    _main_xrun_progress->set_text("JACK stopped");
    return;
  }

  char tmp_buf[8];
  snprintf(tmp_buf, 8, "%zd", _jack->xruns());

  _main_xrun_progress->set_text(std::string(tmp_buf) + " Dropouts");

  float load = _jack->get_dsp_load();

  load /= 100.0;                // dbus returns it in percents, we use 0..1

  if (load > _max_dsp_load)
  {
    _max_dsp_load = load;
    _main_xrun_progress->set_fraction(load);
  }
#endif
}


void
Patchage::zoom(double z)
{
//   _canvas->set_zoom(z);
}


void
Patchage::refresh() 
{
//   assert(_canvas);

//   _canvas->destroy();

//   if (_jack)
//     _jack->refresh();

//   for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
//     (*i)->resize();
//   }
}

void
Patchage::clear_load()
{
#if 0
  _main_xrun_progress->set_fraction(0.0);
  _jack->reset_xruns();
  _max_dsp_load = 0.0;
#endif
}

void
Patchage::update_state()
{
//   for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
//     shared_ptr<Module> module = dynamic_pointer_cast<Module>(*i);
//     if (module) 
//       module->load_location();
//   }
}


/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Patchage is already connected to Jack)
 */
void
Patchage::connect_widgets()
{
  //_jack->signal_started.connect(sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), true));
  //_jack->signal_stopped.connect(sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), false));
}

void
Patchage::jack_status_changed(
  bool started)
{
  update_toolbar();

  _menu_jack_start->set_sensitive(!started);
  _menu_jack_stop->set_sensitive(started);
  _clear_load_but->set_sensitive(started);
  if (!started)
  {
    _main_xrun_progress->set_fraction(0.0);
  }
}

void
Patchage::on_arrange() 
{
//   assert(_canvas);
  
//   _canvas->arrange();
}

  
void
Patchage::on_help_about() 
{
  _about_win->run();
  _about_win->hide();
}


void
Patchage::on_quit() 
{
  _main_win->hide();
}

void
Patchage::on_show_projects()
{
  if (_menu_view_projects->get_active())
    _project_list_viewport->show();
  else
    _project_list_viewport->hide();
}

void
Patchage::on_view_toolbar() 
{
  if (_menu_view_toolbar->get_active())
    _toolbar->show();
  else
    _toolbar->hide();
}

  
bool
Patchage::on_scroll(GdkEventScroll* ev) 
{
  lash_debug("ON SCROLL");
  return false;
}


void
Patchage::buffer_size_changed()
{
#if 0
  const int selected = _buffer_size_combo->get_active_row_number();

  if (selected == -1)
  {
    update_toolbar();
  }
  else
  {
    uint32_t buffer_size = 1 << (selected+5);
  
    // this check is temporal workaround for jack bug
    // we skip setting buffer size if it same as acutal one
    // proper place for such check is in jack
    if (_jack->buffer_size() != buffer_size)
    {
      if (!_jack->set_buffer_size(buffer_size))
      {
        update_toolbar(); // reset combo box to actual value
      }
    }
  }
#endif
}

void
Patchage::set_studio_availability(
  bool available)
{
  if (available)
  {
    _main_win->set_title("Active Studio - LADI Session Handler");
  }
  else
  {
    _main_win->set_title("LADI Session Handler");
  }
  //_project_list->set_lash_availability(available);
  _menu_view_projects->set_active(available);
}

#if 0
void
Patchage::set_a2j_status(
  unsigned int status)
{
  const char * status_text;

  switch (status)
  {
  case A2J_STATUS_NO_RESPONSE:
    status_text = "A2J N/A";
    _menu_a2j_start->set_sensitive(false);
    _menu_a2j_stop->set_sensitive(false);
    break;
  case A2J_STATUS_BRIDGE_STOPPED:
    status_text = "A2J bridge stopped";
    _menu_a2j_start->set_sensitive(true);
    _menu_a2j_stop->set_sensitive(false);
    break;
  case A2J_STATUS_BRIDGE_STARTED:
    status_text = "A2J bridge started";
    _menu_a2j_start->set_sensitive(false);
    _menu_a2j_stop->set_sensitive(true);
    break;
  default:
    error_msg(str(boost::format("Unknown A2J status %u") % status));
    status_text = "Unknown A2J status";
    _menu_a2j_start->set_sensitive(true);
    _menu_a2j_stop->set_sensitive(true);
    break;
  }

  _main_a2j_status_label->set_text(status_text);
}

void
Patchage::load_project_ask()
{
  std::list<lash_project_info> projects;

  _lash->get_available_projects(projects);
  run_load_project_dialog(projects);
}

void
Patchage::load_project(
  const std::string& project_name)
{
  _lash->load_project(project_name);
}

void
Patchage::save_all_projects()
{
  _lash->save_all_projects();
}

void
Patchage::save_project(
  const std::string& project_name)
{
  _lash->save_project(project_name);
}

void
Patchage::close_project(
  const std::string& project_name)
{
  _lash->close_project(project_name);
}

void
Patchage::close_all_projects()
{
  _lash->close_all_projects();
}

void
Patchage::on_port_added(
  const char * jack_client_name,
  const char * jack_port_name,
  PortType port_type,
  bool is_input,
  bool is_terminal)
{
  bool is_a2j_mapped;
  std::string canvas_client_name;
  std::string canvas_port_name;
  uint32_t alsa_client_id;
  boost::shared_ptr<PatchageModule> module;

  is_a2j_mapped = strcmp(_a2j->get_jack_client_name(), jack_client_name) == 0;
  if (is_a2j_mapped)
  {
    if (!_a2j->map_jack_port(jack_port_name, canvas_client_name, canvas_port_name, alsa_client_id))
    {
      return;
    }

    canvas_port_name = str(boost::format(canvas_port_name + " [a2j:%u]") % alsa_client_id);
  }
  else
  {
    canvas_client_name = jack_client_name;
    canvas_port_name = jack_port_name;
  }

  ModuleType module_type = InputOutput;
  if (_state_manager->get_module_split(canvas_client_name, is_terminal && !is_a2j_mapped)) {
    if (is_input) {
      module_type = Input;
    } else {
      module_type = Output;
    }
  }

  module = _canvas->find_module(canvas_client_name, module_type);
  if (!module) {
    module = boost::shared_ptr<PatchageModule>(new PatchageModule(this, canvas_client_name, module_type));
    module->load_location();
    _canvas->add_item(module);
  }

  if (module->get_port(canvas_port_name)) {
    return;
  }

  boost::shared_ptr<PatchagePort> port = boost::shared_ptr<PatchagePort>(
    new PatchagePort(
      module,
      canvas_port_name,
      is_input,
      _state_manager->get_port_color(port_type)));

  port->type = port_type;
  port->is_a2j_mapped = is_a2j_mapped;
  if (is_a2j_mapped)
  {
    port->a2j_jack_port_name = jack_port_name;
  }

  module->add_port(port);

  module->resize();
}

boost::shared_ptr<PatchagePort>
Patchage::lookup_port(
  const char * jack_client_name,
  const char * jack_port_name)
{
  if (strcmp(_a2j->get_jack_client_name(), jack_client_name) == 0)
  {
    return _canvas->lookup_port_by_a2j_jack_port_name(jack_port_name);
  }

  return _canvas->get_port(jack_client_name, jack_port_name);
}

void
Patchage::on_port_removed(
  const char * jack_client_name,
  const char * jack_port_name)
{
  boost::shared_ptr<PatchagePort> port = lookup_port(jack_client_name, jack_port_name);
  if (!port) {
    error_msg(str(boost::format("Unable to remove unknown port '%s':'%s'") % jack_client_name % jack_port_name));
    return;
  }

  boost::shared_ptr<PatchageModule> module = dynamic_pointer_cast<PatchageModule>(port->module().lock());

  module->remove_port(port);
  port.reset();
      
  // No empty modules (for now)
  if (module->num_ports() == 0) {
    _canvas->remove_item(module);
    module.reset();
  } else {
    module->resize();
  }
}

void
Patchage::on_ports_connected(
  const char * jack_client1_name,
  const char * jack_port1_name,
  const char * jack_client2_name,
  const char * jack_port2_name)
{
  boost::shared_ptr<PatchagePort> port1 = lookup_port(jack_client1_name, jack_port1_name);
  if (!port1) {
    error_msg((std::string)"Unable to connect unknown port '" + jack_port1_name + "' of client '" + jack_client1_name + "'");
    return;
  }

  boost::shared_ptr<PatchagePort> port2 = lookup_port(jack_client2_name, jack_port2_name);
  if (!port2) {
    error_msg((std::string)"Unable to connect unknown port '" + jack_port2_name + "' of client '" + jack_client2_name + "'");
    return;
  }

  _canvas->add_connection(port1, port2, port1->color() + 0x22222200);
}

void
Patchage::on_ports_disconnected(
  const char * jack_client1_name,
  const char * jack_port1_name,
  const char * jack_client2_name,
  const char * jack_port2_name)
{
  boost::shared_ptr<PatchagePort> port1 = lookup_port(jack_client1_name, jack_port1_name);
  if (!port1) {
    error_msg((std::string)"Unable to disconnect unknown port '" + jack_port1_name + "' of client '" + jack_client1_name + "'");
    return;
  }

  boost::shared_ptr<PatchagePort> port2 = lookup_port(jack_client2_name, jack_port2_name);
  if (!port2) {
    error_msg((std::string)"Unable to disconnect unknown port '" + jack_port2_name + "' of client '" + jack_client2_name + "'");
    return;
  }

  _canvas->remove_connection(port1, port2);
}

static
bool
port_type_match(
  boost::shared_ptr<PatchagePort> port1,
  boost::shared_ptr<PatchagePort> port2)
{
  return port1->type == port2->type;
}

void
Patchage::get_port_jack_names(
  boost::shared_ptr<PatchagePort> port,
  std::string& jack_client_name,
  std::string& jack_port_name)
{
  if (port->is_a2j_mapped)
  {
    jack_client_name = _a2j->get_jack_client_name();
    jack_port_name = port->a2j_jack_port_name;
  }
  else
  {
    jack_client_name = port->module().lock()->name();
    jack_port_name = port->name();
  }
}

void
Patchage::connect(
  boost::shared_ptr<PatchagePort> port1,
  boost::shared_ptr<PatchagePort> port2)
{
  std::string jack_client1_name;
  std::string jack_port1_name;
  std::string jack_client2_name;
  std::string jack_port2_name;

  if (port_type_match(port1, port2))
  {
    get_port_jack_names(port1, jack_client1_name, jack_port1_name);
    get_port_jack_names(port2, jack_client2_name, jack_port2_name);

    _jack->connect(
      jack_client1_name.c_str(),
      jack_port1_name.c_str(),
      jack_client2_name.c_str(),
      jack_port2_name.c_str());
  }
  else
  {
    status_msg("ERROR: Attempt to connect ports with mismatched types");
  }
}

void
Patchage::disconnect(
  boost::shared_ptr<PatchagePort> port1,
  boost::shared_ptr<PatchagePort> port2)
{
  std::string jack_client1_name;
  std::string jack_port1_name;
  std::string jack_client2_name;
  std::string jack_port2_name;

  if (port_type_match(port1, port2))
  {
    get_port_jack_names(port1, jack_client1_name, jack_port1_name);
    get_port_jack_names(port2, jack_client2_name, jack_port2_name);

    _jack->disconnect(
      jack_client1_name.c_str(),
      jack_port1_name.c_str(),
      jack_client2_name.c_str(),
      jack_port2_name.c_str());
  }
  else
  {
    status_msg("ERROR: Attempt to disconnect ports with mismatched types");
  }
}

bool
Patchage::is_canvas_empty()
{
  return _canvas->items().empty();
}
#endif
#endif

GtkScrolledWindow * g_main_scrolledwin;

struct view
{
  char * name;
  graph_canvas_handle graph_canvas;
  graph_handle graph;
  GtkWidget * canvas_widget;
};

struct view * create_view(const char * name, const char * service, const char * object)
{
  struct view * view_ptr;

  view_ptr = malloc(sizeof(struct view));
  if (view_ptr == NULL)
  {
    lash_error("malloc() failed for struct view");
    goto fail;
  }

  view_ptr->name = strdup(name);
  if (view_ptr->name == NULL)
  {
    lash_error("strdup() failed for \"%s\"", name);
    goto free_view;
  }

  if (!graph_create(service, object, &view_ptr->graph))
  {
    goto free_name;
  }

  if (!graph_canvas_create(1600 * 2, 1200 * 2, &view_ptr->graph_canvas))
  {
    goto destroy_graph;
  }

  if (!graph_canvas_attach(view_ptr->graph_canvas, view_ptr->graph))
  {
    goto destroy_graph_canvas;
  }

  if (!graph_activate(view_ptr->graph))
  {
    goto detach_graph_canvas;
  }

  view_ptr->canvas_widget = canvas_get_widget(graph_canvas_get_canvas(view_ptr->graph_canvas));

  gtk_widget_show(view_ptr->canvas_widget);

  return view_ptr;

detach_graph_canvas:
  graph_canvas_detach(view_ptr->graph_canvas);
destroy_graph_canvas:
  graph_canvas_destroy(view_ptr->graph_canvas);
destroy_graph:
  graph_destroy(view_ptr->graph);
free_name:
  free(view_ptr->name);
free_view:
  free(view_ptr);
fail:
  return NULL;
}

void destroy_view(struct view * view_ptr)
{
  graph_canvas_detach(view_ptr->graph_canvas);
  graph_canvas_destroy(view_ptr->graph_canvas);
  graph_destroy(view_ptr->graph);
  free(view_ptr->name);
  free(view_ptr);
}

void attach_view(struct view * view_ptr)
{
  GtkWidget * child;

  child = gtk_bin_get_child(GTK_BIN(g_main_scrolledwin));
  if (child != NULL)
  {
    gtk_container_remove(GTK_CONTAINER(g_main_scrolledwin), child);
  }

  gtk_container_add(GTK_CONTAINER(g_main_scrolledwin), view_ptr->canvas_widget);

  //_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320), static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded
  //_main_scrolledwin->property_hadjustment().get_value()->set_step_increment(10);
  //_main_scrolledwin->property_vadjustment().get_value()->set_step_increment(10);
}

void detach_view(struct view * view_ptr)
{
  GtkWidget * child;

  child = gtk_bin_get_child(GTK_BIN(g_main_scrolledwin));
  if (child == view_ptr->canvas_widget)
  {
    gtk_container_remove(GTK_CONTAINER(g_main_scrolledwin), view_ptr->canvas_widget);
  }
}

struct view * g_jack_view;
struct view * g_studio_view;

void control_proxy_on_studio_appeared(void)
{
  g_studio_view = create_view("Studio", SERVICE_NAME, STUDIO_OBJECT_PATH);
  attach_view(g_studio_view);
  world_tree_add_studio(g_studio_view->graph);

  g_jack_view = create_view("Raw JACK", JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH);
  attach_view(g_jack_view);
  world_tree_add_jack(g_jack_view->graph);
}

void control_proxy_on_studio_disappeared(void)
{
  detach_view(g_jack_view);
  world_tree_remove(g_jack_view->graph);
  destroy_view(g_jack_view);

  detach_view(g_studio_view);
  world_tree_remove(g_studio_view->graph);
  destroy_view(g_studio_view);
}

int main(int argc, char** argv)
{
  GtkWidget * main_win;

  gtk_init(&argc, &argv);

  if (!canvas_init())
  {
    lash_error("Canvas initialization failed.");
    return 1;
  }

  if (!init_glade())
  {
    return 1;
  }

  /* Obtain widgets that we need */
  main_win = get_glade_widget("main_win");
  g_main_scrolledwin = GTK_SCROLLED_WINDOW(get_glade_widget("main_scrolledwin"));

  world_tree_init();

  patchage_dbus_init();

  if (!control_proxy_init())
  {
    return 1;
  }

  //gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);

  g_signal_connect(G_OBJECT(main_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(get_glade_widget("menu_file_quit")), "activate", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show(main_win);

  //_about_win->set_transient_for(*_main_win);

  gtk_main();

  control_proxy_uninit();
  uninit_glade();

  return 0;
}
