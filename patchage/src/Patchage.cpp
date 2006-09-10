/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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


Patchage::Patchage(int argc, char** argv)
: 
#ifdef HAVE_LASH
  m_lash_driver(NULL),
#endif
#ifdef HAVE_ALSA
  m_alsa_driver(NULL),
#endif
  m_canvas(NULL),
  m_jack_driver(NULL),
  m_state_manager(NULL),
  m_refresh(false)
{
	m_settings_filename = getenv("HOME");
	m_settings_filename += "/.patchagerc";

	m_state_manager = new StateManager();
	m_canvas = new PatchageFlowCanvas(this, 1600*2, 1200*2);
	m_jack_driver = new JackDriver(this);
#ifdef HAVE_ALSA
	m_alsa_driver = new AlsaDriver(this);
#endif
	
	m_state_manager->load(m_settings_filename);

#ifdef HAVE_LASH
	m_lash_driver = new LashDriver(this, argc, argv);
#endif

	Glib::RefPtr<Gnome::Glade::Xml> refXml;

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
		refXml = Gnome::Glade::Xml::create(glade_filename);
	} catch(const Gnome::Glade::XmlError& ex) {
		std::cerr << ex.what() << std::endl;
		throw;
	}

	refXml->get_widget("patchage_win", m_main_window);
	refXml->get_widget("about_win", m_about_window);
	refXml->get_widget("about_project_label", m_about_project_label);
	refXml->get_widget("launch_jack_menuitem", m_menu_jack_launch);
	refXml->get_widget("connect_to_jack_menuitem", m_menu_jack_connect);
	refXml->get_widget("disconnect_from_jack_menuitem", m_menu_jack_disconnect);
#ifdef HAVE_LASH
	refXml->get_widget("launch_lash_menuitem", m_menu_lash_launch);
	refXml->get_widget("connect_to_lash_menuitem", m_menu_lash_connect);
	refXml->get_widget("disconnect_from_lash_menuitem", m_menu_lash_disconnect);
#endif
#ifdef HAVE_ALSA
	refXml->get_widget("connect_to_alsa_menuitem", m_menu_alsa_connect);
	refXml->get_widget("disconnect_from_alsa_menuitem", m_menu_alsa_disconnect);
#endif
	refXml->get_widget("file_save_menuitem", m_menu_file_save);
	refXml->get_widget("file_quit_menuitem", m_menu_file_quit);
	refXml->get_widget("view_refresh_menuitem", m_menu_view_refresh);
	refXml->get_widget("help_about_menuitem", m_menu_help_about);
	refXml->get_widget("canvas_scrolledwindow", m_canvas_scrolledwindow);
	refXml->get_widget("zoom_scale", m_zoom_slider);
	refXml->get_widget("about_close_button", m_about_close_button);
	refXml->get_widget("status_lab", m_status_label);
	
	m_main_window->resize(
		static_cast<int>(m_state_manager->get_window_size().x),
		static_cast<int>(m_state_manager->get_window_size().y));
	
	m_main_window->move(
		static_cast<int>(m_state_manager->get_window_location().x),
		static_cast<int>(m_state_manager->get_window_location().y));

	m_canvas_scrolledwindow->add(*m_canvas);
	//m_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(m_canvas, &FlowCanvas::scroll_event_handler));
	m_canvas->scroll_to(static_cast<int>(m_canvas->width()/2 - 320),
	                       static_cast<int>(m_canvas->height()/2 - 240)); // FIXME: hardcoded
	m_canvas->show();

	// Idle callback, check if we need to refresh (every 250msec)
	Glib::signal_timeout().connect(sigc::mem_fun(this, &Patchage::idle_callback), 250);
	
	m_zoom_slider->signal_value_changed().connect(    sigc::mem_fun(this, &Patchage::zoom_changed));
	m_menu_jack_launch->signal_activate().connect(    sigc::mem_fun(this, &Patchage::menu_jack_launch));
	m_menu_jack_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_jack_connect));
	m_menu_jack_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_jack_disconnect));
#ifdef HAVE_LASH
	m_menu_lash_launch->signal_activate().connect(    sigc::mem_fun(this, &Patchage::menu_lash_launch));
	m_menu_lash_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_lash_connect));
	m_menu_lash_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_lash_disconnect));
#endif
	m_menu_alsa_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_alsa_connect));
	m_menu_alsa_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_alsa_disconnect));
	m_menu_file_save->signal_activate().connect(      sigc::mem_fun(this, &Patchage::menu_file_save));
	m_menu_file_quit->signal_activate().connect(      sigc::mem_fun(this, &Patchage::menu_file_quit));
	m_menu_view_refresh->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_view_refresh));
	m_menu_help_about->signal_activate().connect(     sigc::mem_fun(this, &Patchage::menu_help_about));
	m_about_close_button->signal_clicked().connect(   sigc::mem_fun(this, &Patchage::close_about));

	//_about_project_label->use_markup(true);
	m_about_project_label->set_markup("<span size=\"xx-large\" weight=\"bold\">Patchage " PACKAGE_VERSION "</span>");
}


Patchage::~Patchage() 
{
#ifdef HAVE_LASH
	delete m_lash_driver;
#endif
	delete m_jack_driver;
	delete m_alsa_driver;
	delete m_canvas;
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

	update_menu_items();
	menu_view_refresh();
}
	

bool
Patchage::idle_callback() 
{
	// FIXME: no need to destroy the whole canvas every time
	if (m_refresh || (m_jack_driver && m_jack_driver->is_dirty())
#ifdef HAVE_ALSA
	              || (m_alsa_driver && m_alsa_driver->is_dirty())
#endif
		) {
		m_canvas->destroy();
		m_jack_driver->refresh();
#ifdef HAVE_ALSA
		m_alsa_driver->refresh();
#endif
		update_menu_items();
		m_refresh = false;
	}

#ifdef HAVE_LASH
	if (m_lash_driver->is_attached())
		m_lash_driver->process_events();
#endif

	return true;
}


void
Patchage::zoom_changed() 
{
	const double z = m_zoom_slider->get_value();
	
	m_canvas->set_zoom(z);
	m_state_manager->set_zoom(z);
}


void
Patchage::update_state()
{
	for (ModuleMap::iterator i = m_canvas->modules().begin(); i != m_canvas->modules().end(); ++i)
		(*i).second->load_location();

	cerr << "[Patchage] Resizing window: (" << m_state_manager->get_window_size().x
		<< "," << m_state_manager->get_window_size().y << ")" << endl;

	m_main_window->resize(
		static_cast<int>(m_state_manager->get_window_size().x),
		static_cast<int>(m_state_manager->get_window_size().y));
	
	cerr << "[Patchage] Moving window: (" << m_state_manager->get_window_location().x
		<< "," << m_state_manager->get_window_location().y << ")" << endl;
	m_main_window->move(
		static_cast<int>(m_state_manager->get_window_location().x),
		static_cast<int>(m_state_manager->get_window_location().y));
}


void
Patchage::status_message(const string& msg) 
{
	m_status_label->set_text(msg);
}




/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Patchage is already connected to Jack)
 */
void
Patchage::update_menu_items()
{
	// Update Jack menu items
	const bool jack_attached = m_jack_driver->is_attached();
	m_menu_jack_launch->set_sensitive(!jack_attached);
	m_menu_jack_connect->set_sensitive(!jack_attached);
	m_menu_jack_disconnect->set_sensitive(jack_attached);
	
	// Update Lash menu items
#ifdef HAVE_LASH
	const bool lash_attached = m_lash_driver->is_attached();
	m_menu_lash_launch->set_sensitive(!lash_attached);
	m_menu_lash_connect->set_sensitive(!lash_attached);
	m_menu_lash_disconnect->set_sensitive(lash_attached);
#endif

#ifdef HAVE_ALSA
	// Update Alsa menu items
	const bool alsa_attached = m_alsa_driver->is_attached();
	m_menu_alsa_connect->set_sensitive(!alsa_attached);
	m_menu_alsa_disconnect->set_sensitive(alsa_attached);
#endif
}


void
Patchage::menu_jack_launch() 
{
	m_jack_driver->attach(true);
	update_menu_items();
}


void
Patchage::menu_jack_connect() 
{
	m_jack_driver->attach(false);
	update_menu_items();
}


void
Patchage::menu_jack_disconnect() 
{
	m_jack_driver->detach();
	menu_view_refresh();
	update_menu_items();
}

#ifdef HAVE_LASH
void
Patchage::menu_lash_launch() 
{
	m_lash_driver->attach(true);
	update_menu_items();
}


void
Patchage::menu_lash_connect() 
{
	m_lash_driver->attach(false);
	update_menu_items();
}


void
Patchage::menu_lash_disconnect() 
{
	m_lash_driver->detach();
	update_menu_items();
}
#endif

#ifdef HAVE_ALSA
void
Patchage::menu_alsa_connect() 
{
	m_alsa_driver->attach(false);
	update_menu_items();
}


void
Patchage::menu_alsa_disconnect() 
{
	m_alsa_driver->detach();
	menu_view_refresh();
	update_menu_items();
}
#endif

void
Patchage::menu_file_save() 
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
Patchage::menu_view_refresh() 
{
	assert(m_canvas);
	
	// FIXME: rebuilding the entire canvas each time is garbage
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


void
Patchage::close_about() 
{
	m_about_window->hide();
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


