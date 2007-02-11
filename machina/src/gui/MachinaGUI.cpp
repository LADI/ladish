/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cmath>
#include <sstream>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <fstream>
#include <pthread.h>
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"

//#include "config.h"

// FIXME: include to avoid undefined reference to boost SP debug hooks stuff
#include <raul/SharedPtr.h>



/* Gtk helpers (resize combo boxes) */

#if 0
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
#endif
/* end Gtk helpers */



MachinaGUI::MachinaGUI(SharedPtr<Machina::Machine> machine)
: _pane_closed(false),
  _update_pane_position(true),
  _user_pane_position(0),
  _refresh(false),
  _machine(machine),
  _maid(new Raul::Maid(32))
{
	/*_settings_filename = getenv("HOME");
	_settings_filename += "/.machinarc";*/

	_canvas = boost::shared_ptr<MachinaCanvas>(new MachinaCanvas(this, 1600*2, 1200*2));

	Glib::RefPtr<Gnome::Glade::Xml> xml;

	// Check for the .glade file in current directory
	string glade_filename = "./machina.glade";
	ifstream fs(glade_filename.c_str());
	if (fs.fail()) { // didn't find it, check PKGDATADIR
		fs.clear();
		glade_filename = PKGDATADIR;
		glade_filename += "/machina.glade";
	
		fs.open(glade_filename.c_str());
		if (fs.fail()) {
			cerr << "Unable to find machina.glade in current directory or " << PKGDATADIR << "." << endl;
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

	xml->get_widget("machina_win", _main_window);
	xml->get_widget("about_win", _about_window);
	xml->get_widget("help_dialog", _help_dialog);
	xml->get_widget("file_quit_menuitem", _menu_file_quit);
	xml->get_widget("view_refresh_menuitem", _menu_view_refresh);
	xml->get_widget("view_messages_menuitem", _menu_view_messages);
	xml->get_widget("help_about_menuitem", _menu_help_about);
	xml->get_widget("help_help_menuitem", _menu_help_help);
	xml->get_widget("canvas_scrolledwindow", _canvas_scrolledwindow);
	xml->get_widget("status_text", _status_text);
	xml->get_widget("main_paned", _main_paned);
	xml->get_widget("messages_expander", _messages_expander);
	xml->get_widget("zoom_full_but", _zoom_full_button);
	xml->get_widget("zoom_normal_but", _zoom_normal_button);
	
	_canvas_scrolledwindow->add(*_canvas);
	//m_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(_canvas, &FlowCanvas::scroll_event_handler));
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                       static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	//_zoom_slider->signal_value_changed().connect(sigc::mem_fun(this, &MachinaGUI::zoom_changed));
	
	_zoom_normal_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::zoom), 1.0));
	
	_zoom_full_button->signal_clicked().connect(sigc::mem_fun(_canvas.get(), &MachinaCanvas::zoom_full));

	_menu_file_quit->signal_activate().connect(      sigc::mem_fun(this, &MachinaGUI::menu_file_quit));
	_menu_view_refresh->signal_activate().connect(   sigc::mem_fun(this, &MachinaGUI::menu_view_refresh));
	_menu_view_messages->signal_toggled().connect(   sigc::mem_fun(this, &MachinaGUI::show_messages_toggled));
	_menu_help_about->signal_activate().connect(     sigc::mem_fun(this, &MachinaGUI::menu_help_about));
	_menu_help_help->signal_activate().connect(     sigc::mem_fun(this, &MachinaGUI::menu_help_help));

	connect_widgets();
	
	//update_state();

	_canvas->show();

	_main_window->present();

	_update_pane_position = false;
	_main_paned->set_position(max_pane_position());

	_main_paned->property_position().signal_changed().connect(
		sigc::mem_fun(*this, &MachinaGUI::on_pane_position_changed));
	
	_messages_expander->property_expanded().signal_changed().connect(
		sigc::mem_fun(*this, &MachinaGUI::on_messages_expander_changed));
	
	_main_paned->set_position(max_pane_position());
	_user_pane_position = max_pane_position() - _main_window->get_height()/8;
	_update_pane_position = true;
	_pane_closed = true;

	// Idle callback to drive the maid (collect garbage)
	Glib::signal_timeout().connect(
		sigc::bind_return(
			sigc::mem_fun(_maid.get(), &Raul::Maid::cleanup),
			true),
		1000);
	
	// Idle callback to update node states
	Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::idle_callback), 100);
	
	// Faster idle callback to update DSP load progress bar
	//Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::update_load), 50);
}


MachinaGUI::~MachinaGUI() 
{
}


void
MachinaGUI::attach()
{
#if 0
	_jack_driver->attach(true);

	menu_view_refresh();

	update_toolbar();

	//m_status_bar->push("Connected to JACK server");
#endif
}
	

bool
MachinaGUI::idle_callback() 
{
	for (ItemMap::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		SharedPtr<NodeView> nv = PtrCast<NodeView>(i->second);
		if (nv)
			nv->update_state();
	}

	return true;
}


void
MachinaGUI::zoom(double z)
{
	_canvas->set_zoom(z);
}


void
MachinaGUI::zoom_changed() 
{
/*
	static bool enable_signal = true;
	if (enable_signal) {
		enable_signal = false;
		zoom(_zoom_slider->get_value());
		enable_signal = true;
	}
	*/
}


#if 0
void
MachinaGUI::update_state()
{
	for (ModuleMap::iterator i = _canvas->modules().begin(); i != _canvas->modules().end(); ++i)
		(*i).second->load_location();

	//cerr << "[Machina] Resizing window: (" << _state_manager->get_window_size().x
	//	<< "," << _state_manager->get_window_size().y << ")" << endl;

	_main_window->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));
	
	//cerr << "[Machina] Moving window: (" << _state_manager->get_window_location().x
	//	<< "," << _state_manager->get_window_location().y << ")" << endl;
	
	_main_window->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));

}

#endif


void
MachinaGUI::status_message(const string& msg) 
{
	if (_status_text->get_buffer()->size() > 0)
		_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), "\n");

	_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), msg);
	_status_text->scroll_to_mark(_status_text->get_buffer()->get_insert(), 0);
}


/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Machina is already connected to Jack)
 */
void
MachinaGUI::connect_widgets()
{

}


void
MachinaGUI::menu_file_quit() 
{
	_main_window->hide();
}


void
MachinaGUI::on_pane_position_changed()
{
	// avoid infinite recursion...
	if (!_update_pane_position)
		return;

	_update_pane_position = false;

	int new_position = _main_paned->get_position();

	if (_pane_closed && new_position < max_pane_position()) {
		// Auto open
		_user_pane_position = new_position;
		_messages_expander->set_expanded(true);
		_pane_closed = false;
		_menu_view_messages->set_active(true);
	} else if (new_position >= max_pane_position()) {
		// Auto close
		_pane_closed = true;

		_messages_expander->set_expanded(false);
		if (new_position > max_pane_position())
			_main_paned->set_position(max_pane_position()); // ... here
		_menu_view_messages->set_active(false);
		
		_user_pane_position = max_pane_position() - _main_window->get_height()/8;
	}

	_update_pane_position = true;
}


void
MachinaGUI::on_messages_expander_changed()
{
	if (!_pane_closed) {
		// Store pane position for restoring
		_user_pane_position = _main_paned->get_position();
		if (_update_pane_position) {
			_update_pane_position = false;
			_main_paned->set_position(max_pane_position());
			_update_pane_position = true;
		}
		_pane_closed = true;
	} else {
		_main_paned->set_position(_user_pane_position);
		_pane_closed = false;
	}
}


void
MachinaGUI::show_messages_toggled()
{
	if (_update_pane_position)
		_messages_expander->set_expanded(_menu_view_messages->get_active());
}


void
MachinaGUI::menu_view_refresh() 
{
	assert(_canvas);
	
	//_canvas->destroy();
}


void
MachinaGUI::menu_help_about() 
{
	_about_window->show();
}


void
MachinaGUI::menu_help_help() 
{
	_help_dialog->run();
	_help_dialog->hide();
}
