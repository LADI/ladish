/* This file is part of Patchage.  Copyright (C) 2006 Dave Robillard.
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Patchage.h"
#include "config.h"
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <fstream>
#include <pthread.h>
#include "StateManager.h"
#include "PatchageFlowCanvas.h"
#include "AlsaDriver.h"
#include "JackDriver.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif

// FIXME: include to avoid undefined reference to boost SP debug hooks stuff
#include <raul/SharedPtr.h>

Patchage::Patchage(int argc, char** argv)
: 
#ifdef HAVE_LASH
  m_lash_driver(NULL),
#endif
#ifdef HAVE_ALSA
  m_alsa_driver(NULL),
#endif
  m_jack_driver(NULL),
  m_state_manager(NULL),
  m_refresh(false)
{
	m_settings_filename = getenv("HOME");
	m_settings_filename += "/.patchagerc";

	m_state_manager = new StateManager();
	m_canvas = boost::shared_ptr<PatchageFlowCanvas>(new PatchageFlowCanvas(this, 1600*2, 1200*2));
	m_jack_driver = new JackDriver(this);
	m_jack_driver->signal_detached.connect(sigc::mem_fun(this, &Patchage::queue_refresh));

#ifdef HAVE_ALSA
	m_alsa_driver = new AlsaDriver(this);
#endif
	
	m_state_manager->load(m_settings_filename);

#ifdef HAVE_LASH
	m_lash_driver = new LashDriver(this, argc, argv);
#endif

	Glib::RefPtr<Gnome::Glade::Xml> xml;

	// Check for the .glade file in current directory
	string glade_filename = "./patchage.glade";
	ifstream fs(glade_filename.c_str());
	if (fs.fail()) { // didn't find it, check PKGDATADIR
		fs.clear();
		glade_filename = PKGDATADIR;
		glade_filename += "/patchage.glade";
	
		fs.open(glade_filename.c_str());
		if (fs.fail()) {
			cerr << "Unable to find patchage.glade in current directory or " << PKGDATADIR << "." << endl;
			exit(EXIT_FAILURE);
		}
		fs.close();
	}
	
	try {
		xml = Gnome::Glade::Xml::create(glade_filename);
	} catch(const Gnome::Glade::XmlError& ex) {
		std::cerr << ex.what() << std::endl;
		throw;
	}

	xml->get_widget("patchage_win", m_main_window);
	xml->get_widget("about_win", m_about_window);
	xml->get_widget("launch_jack_menuitem", m_menu_jack_launch);
	xml->get_widget("connect_to_jack_menuitem", m_menu_jack_connect);
	xml->get_widget("disconnect_from_jack_menuitem", m_menu_jack_disconnect);
#ifdef HAVE_LASH
	xml->get_widget("launch_lash_menuitem", m_menu_lash_launch);
	xml->get_widget("connect_to_lash_menuitem", m_menu_lash_connect);
	xml->get_widget("disconnect_from_lash_menuitem", m_menu_lash_disconnect);
#endif
#ifdef HAVE_ALSA
	xml->get_widget("connect_to_alsa_menuitem", m_menu_alsa_connect);
	xml->get_widget("disconnect_from_alsa_menuitem", m_menu_alsa_disconnect);
#endif
	xml->get_widget("store_positions_menuitem", m_menu_store_positions);
	xml->get_widget("file_quit_menuitem", m_menu_file_quit);
	xml->get_widget("view_refresh_menuitem", m_menu_view_refresh);
	xml->get_widget("view_messages_menuitem", m_menu_view_messages);
	xml->get_widget("help_about_menuitem", m_menu_help_about);
	xml->get_widget("canvas_scrolledwindow", m_canvas_scrolledwindow);
	xml->get_widget("zoom_scale", m_zoom_slider);
	xml->get_widget("status_text", m_status_text);
	xml->get_widget("main_paned", m_main_paned);
	xml->get_widget("zoom_full_but", m_zoom_full_button);
	xml->get_widget("zoom_normal_but", m_zoom_normal_button);
	
	m_canvas_scrolledwindow->add(*m_canvas);
	//m_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(m_canvas, &FlowCanvas::scroll_event_handler));
	m_canvas->scroll_to(static_cast<int>(m_canvas->width()/2 - 320),
	                       static_cast<int>(m_canvas->height()/2 - 240)); // FIXME: hardcoded

	m_zoom_slider->signal_value_changed().connect(sigc::mem_fun(this, &Patchage::zoom_changed));
	m_zoom_normal_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &Patchage::zoom), 1.0));

	m_menu_jack_launch->signal_activate().connect(sigc::bind(
		sigc::mem_fun(m_jack_driver, &JackDriver::attach), true));
	
	m_menu_jack_connect->signal_activate().connect(sigc::bind(
		sigc::mem_fun(m_jack_driver, &JackDriver::attach), false));
	
	m_menu_jack_disconnect->signal_activate().connect(sigc::mem_fun(m_jack_driver, &JackDriver::detach));

#ifdef HAVE_LASH
	m_menu_lash_launch->signal_activate().connect(    sigc::mem_fun(this, &Patchage::menu_lash_launch));
	m_menu_lash_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_lash_connect));
	m_menu_lash_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_lash_disconnect));
#endif
	m_menu_alsa_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_alsa_connect));
	m_menu_alsa_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_alsa_disconnect));
	m_menu_store_positions->signal_activate().connect(      sigc::mem_fun(this, &Patchage::menu_store_positions));
	m_menu_file_quit->signal_activate().connect(      sigc::mem_fun(this, &Patchage::menu_file_quit));
	m_menu_view_refresh->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_view_refresh));
	m_menu_view_messages->signal_toggled().connect(   sigc::mem_fun(this, &Patchage::show_messages_toggled));
	m_menu_help_about->signal_activate().connect(     sigc::mem_fun(this, &Patchage::menu_help_about));

	attach_menu_items();
	
	update_state();

	m_canvas->show();

	m_main_paned->set_position(INT_MAX);
	
	// Idle callback, check if we need to refresh
	Glib::signal_timeout().connect(sigc::mem_fun(this, &Patchage::idle_callback), 100);
}


Patchage::~Patchage() 
{
	delete m_jack_driver;
#ifdef HAVE_ALSA
	delete m_alsa_driver;
#endif
#ifdef HAVE_LASH
	delete m_lash_driver;
#endif
	delete m_state_manager;
}


void
Patchage::attach()
{
	m_jack_driver->attach(false);

#ifdef HAVE_LASH
	m_lash_driver->attach(false);
#endif
#ifdef HAVE_ALSA
	m_alsa_driver->attach();
#endif

	menu_view_refresh();
}
	

bool
Patchage::idle_callback() 
{
	bool refresh = m_refresh;

	refresh = refresh || (m_jack_driver && m_jack_driver->is_dirty());
#ifdef HAVE_ALSA
	refresh = refresh || (m_alsa_driver && m_alsa_driver->is_dirty());
#endif

	if (refresh) {
		m_canvas->flag_all_connections();

		m_jack_driver->refresh();
#ifdef HAVE_ALSA
		m_alsa_driver->refresh();
#endif
	}

#ifdef HAVE_LASH
	if (m_lash_driver->is_attached())
		m_lash_driver->process_events();
#endif

	if (refresh) {
		m_canvas->destroy_all_flagged_connections();
		m_refresh = false;
	}

	return true;
}


void
Patchage::zoom(double z)
{
	m_canvas->set_zoom(z);
	m_state_manager->set_zoom(z);
}


void
Patchage::zoom_changed() 
{
	zoom(m_zoom_slider->get_value());
}


void
Patchage::update_state()
{
	for (ModuleMap::iterator i = m_canvas->modules().begin(); i != m_canvas->modules().end(); ++i)
		(*i).second->load_location();

	//cerr << "[Patchage] Resizing window: (" << m_state_manager->get_window_size().x
	//	<< "," << m_state_manager->get_window_size().y << ")" << endl;

	m_main_window->resize(
		static_cast<int>(m_state_manager->get_window_size().x),
		static_cast<int>(m_state_manager->get_window_size().y));
	
	//cerr << "[Patchage] Moving window: (" << m_state_manager->get_window_location().x
	//	<< "," << m_state_manager->get_window_location().y << ")" << endl;
	
	m_main_window->move(
		static_cast<int>(m_state_manager->get_window_location().x),
		static_cast<int>(m_state_manager->get_window_location().y));
}


void
Patchage::status_message(const string& msg) 
{
	if (m_status_text->get_buffer()->size() > 0)
		m_status_text->get_buffer()->insert(m_status_text->get_buffer()->end(), "\n");

	m_status_text->get_buffer()->insert(m_status_text->get_buffer()->end(), msg);
	m_status_text->scroll_to_mark(m_status_text->get_buffer()->get_insert(), 0);
}


/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Patchage is already connected to Jack)
 */
void
Patchage::attach_menu_items()
{
#ifdef HAVE_LASH
	m_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_launch, &Gtk::MenuItem::set_sensitive), false));
	m_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_connect, &Gtk::MenuItem::set_sensitive), false));
	m_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	m_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_launch, &Gtk::MenuItem::set_sensitive), true));
	m_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_connect, &Gtk::MenuItem::set_sensitive), true));
	m_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif

	m_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_launch, &Gtk::MenuItem::set_sensitive), false));
	m_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_connect, &Gtk::MenuItem::set_sensitive), false));
	m_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	m_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_launch, &Gtk::MenuItem::set_sensitive), true));
	m_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_connect, &Gtk::MenuItem::set_sensitive), true));
	m_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), false));
	
	m_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), false));
	m_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(m_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	m_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), true));
	m_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(m_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), false));
}


#ifdef HAVE_LASH
void
Patchage::menu_lash_launch() 
{
	m_lash_driver->attach(true);
}


void
Patchage::menu_lash_connect() 
{
	m_lash_driver->attach(false);
}


void
Patchage::menu_lash_disconnect() 
{
	m_lash_driver->detach();
}
#endif

#ifdef HAVE_ALSA
void
Patchage::menu_alsa_connect() 
{
	m_alsa_driver->attach(false);
}


void
Patchage::menu_alsa_disconnect() 
{
	m_alsa_driver->detach();
	menu_view_refresh();
}
#endif

void
Patchage::menu_store_positions() 
{
	store_window_location();
	m_state_manager->save(m_settings_filename);
}


void
Patchage::menu_file_quit() 
{
#ifdef HAVE_ALSA
	m_alsa_driver->detach();
#endif
	m_jack_driver->detach();
	m_main_window->hide();
}


void
Patchage::show_messages_toggled()
{
	const bool show = m_menu_view_messages->get_active();

	if (show)
		m_main_paned->set_position(static_cast<int>(m_main_paned->get_height() * 3/4));
	else
		m_main_paned->set_position(INT_MAX);
}


void
Patchage::menu_view_refresh() 
{
	assert(m_canvas);
	
	m_canvas->destroy();
	
	if (m_jack_driver)
		m_jack_driver->refresh();

#ifdef HAVE_ALSA
	if (m_alsa_driver)
		m_alsa_driver->refresh();
#endif
}


void
Patchage::menu_help_about() 
{
	m_about_window->show();
}


/** Update the stored window location and size in the StateManager (in memory).
 */
void
Patchage::store_window_location()
{
	int loc_x, loc_y, size_x, size_y;
	m_main_window->get_position(loc_x, loc_y);
	m_main_window->get_size(size_x, size_y);
	Coord window_location;
	window_location.x = loc_x;
	window_location.y = loc_y;
	Coord window_size;
	window_size.x = size_x;
	window_size.y = size_y;
	m_state_manager->set_window_location(window_location);
	m_state_manager->set_window_size(window_size);
}


