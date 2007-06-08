/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cmath>
#include <sstream>
#include "Patchage.h"
#include "PatchageEvent.h"
#include "../config.h"
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <fstream>
#include <pthread.h>
#include "StateManager.h"
#include "PatchageCanvas.h"
#include <jack/statistics.h>
#include "JackDriver.h"
#include "JackSettingsDialog.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
#ifdef HAVE_ALSA
#include "AlsaDriver.h"
#endif

// FIXME: include to avoid undefined reference to boost SP debug hooks stuff
#include <raul/SharedPtr.h>



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



Patchage::Patchage(int argc, char** argv)
: _pane_closed(false),
  _update_pane_position(true),
  _user_pane_position(0),
#ifdef HAVE_LASH
  _lash_driver(NULL),
#endif
#ifdef HAVE_ALSA
  _alsa_driver(NULL),
#endif
  _jack_driver(NULL),
  _state_manager(NULL),
  _refresh(false)
{
	_settings_filename = getenv("HOME");
	_settings_filename += "/.patchagerc";

	_state_manager = new StateManager();
	_canvas = boost::shared_ptr<PatchageCanvas>(new PatchageCanvas(this, 1600*2, 1200*2));
	_jack_driver = new JackDriver(this);
	_jack_driver->signal_detached.connect(sigc::mem_fun(this, &Patchage::queue_refresh));

#ifdef HAVE_ALSA
	_alsa_driver = new AlsaDriver(this);
#endif
	
	_state_manager->load(_settings_filename);

#ifdef HAVE_LASH
	_lash_driver = new LashDriver(this, argc, argv);
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
	
	xml = Gnome::Glade::Xml::create(glade_filename);

	xml->get_widget("patchage_win", _main_window);
	xml->get_widget_derived("jack_settings_win", _jack_settings_dialog);
	xml->get_widget("about_win", _about_window);
	xml->get_widget("jack_settings_menuitem", _menu_jack_settings);
	xml->get_widget("connect_to_jack_menuitem", _menu_jack_connect);
	xml->get_widget("disconnect_from_jack_menuitem", _menu_jack_disconnect);
#ifdef HAVE_LASH
	xml->get_widget("open_session_menuitem", _menu_open_session);
	xml->get_widget("save_session_menuitem", _menu_save_session);
	xml->get_widget("save_session_as_menuitem", _menu_save_session_as);
	xml->get_widget("close_session_menuitem", _menu_close_session);
	xml->get_widget("connect_to_lash_menuitem", _menu_lash_connect);
	xml->get_widget("disconnect_from_lash_menuitem", _menu_lash_disconnect);
#endif
#ifdef HAVE_ALSA
	xml->get_widget("connect_to_alsa_menuitem", _menu_alsa_connect);
	xml->get_widget("disconnect_from_alsa_menuitem", _menu_alsa_disconnect);
#endif
	xml->get_widget("store_positions_menuitem", _menu_store_positions);
	xml->get_widget("file_quit_menuitem", _menu_file_quit);
	xml->get_widget("view_refresh_menuitem", _menu_view_refresh);
	xml->get_widget("view_arrange_menuitem", _menu_view_arrange);
	xml->get_widget("view_messages_menuitem", _menu_view_messages);
	xml->get_widget("view_toolbar_menuitem", _menu_view_toolbar);
	xml->get_widget("help_about_menuitem", _menu_help_about);
	xml->get_widget("toolbar", _toolbar);
	xml->get_widget("canvas_scrolledwindow", _canvas_scrolledwindow);
	xml->get_widget("zoom_scale", _zoom_slider);
	xml->get_widget("status_text", _status_text);
	xml->get_widget("main_paned", _main_paned);
	xml->get_widget("messages_expander", _messages_expander);
	xml->get_widget("rewind_but", _rewind_button);
	xml->get_widget("play_but", _play_button);
	xml->get_widget("stop_but", _stop_button);
	xml->get_widget("zoom_full_but", _zoom_full_button);
	xml->get_widget("zoom_normal_but", _zoom_normal_button);
	//xml->get_widget("main_statusbar", _status_bar);
	//xml->get_widget("main_load_progress", _load_progress_bar);
	//xml->get_widget("main_jack_connect_toggle", _jack_connect_toggle);
	//xml->get_widget("main_jack_realtime_check", _jack_realtime_check);
	xml->get_widget("main_buffer_size_combo", _buffer_size_combo);
	xml->get_widget("main_sample_rate_label", _sample_rate_label);
	xml->get_widget("main_xrun_progress", _xrun_progress_bar);
	xml->get_widget("main_clear_load_button", _clear_load_button);
	
	gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);
	//gtkmm_set_width_for_given_text(*m_sample_rate_combo, "44.1", 40);

	_canvas_scrolledwindow->add(*_canvas);
	//m_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(_canvas, &FlowCanvas::scroll_event_handler));
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                       static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	_zoom_slider->signal_value_changed().connect(sigc::mem_fun(this, &Patchage::zoom_changed));
	
	//_jack_connect_toggle->signal_toggled().connect(sigc::mem_fun(this, &Patchage::jack_connect_changed));

	_buffer_size_combo->signal_changed().connect(sigc::mem_fun(this, &Patchage::buffer_size_changed));
	//m_sample_rate_combo->signal_changed().connect(sigc::mem_fun(this, &Patchage::sample_rate_changed));
	//_jack_realtime_check->signal_toggled().connect(sigc::mem_fun(this, &Patchage::realtime_changed));
	
	_rewind_button->signal_clicked().connect(sigc::mem_fun(_jack_driver, &JackDriver::rewind_transport));
	_play_button->signal_clicked().connect(sigc::mem_fun(_jack_driver, &JackDriver::start_transport));
	_stop_button->signal_clicked().connect(sigc::mem_fun(_jack_driver, &JackDriver::stop_transport));

	_clear_load_button->signal_clicked().connect(sigc::mem_fun(this, &Patchage::clear_load));

	_zoom_normal_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &Patchage::zoom), 1.0));
	
	_zoom_full_button->signal_clicked().connect(sigc::mem_fun(_canvas.get(), &PatchageCanvas::zoom_full));

	_menu_jack_settings->signal_activate().connect(
		sigc::hide_return(sigc::mem_fun(_jack_settings_dialog, &JackSettingsDialog::run)));

	_menu_jack_connect->signal_activate().connect(sigc::bind(
		sigc::mem_fun(_jack_driver, &JackDriver::attach), true));
	
	_menu_jack_disconnect->signal_activate().connect(sigc::mem_fun(_jack_driver, &JackDriver::detach));

#ifdef HAVE_LASH
	_menu_open_session->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_open_session));
	_menu_save_session->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_save_session));
	_menu_save_session_as->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_save_session_as));
	_menu_close_session->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_close_session));
	_menu_lash_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_lash_connect));
	_menu_lash_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_lash_disconnect));
#endif
#ifdef HAVE_ALSA
	_menu_alsa_connect->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_alsa_connect));
	_menu_alsa_disconnect->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_alsa_disconnect));
#endif 
	_menu_store_positions->signal_activate().connect(sigc::mem_fun(this, &Patchage::menu_store_positions));
	_menu_file_quit->signal_activate().connect(      sigc::mem_fun(this, &Patchage::menu_file_quit));
	_menu_view_refresh->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_view_refresh));
	_menu_view_arrange->signal_activate().connect(   sigc::mem_fun(this, &Patchage::menu_view_arrange));
	_menu_view_toolbar->signal_activate().connect(   sigc::mem_fun(this, &Patchage::view_toolbar_toggled));
	_menu_view_messages->signal_toggled().connect(   sigc::mem_fun(this, &Patchage::show_messages_toggled));
	_menu_help_about->signal_activate().connect(     sigc::mem_fun(this, &Patchage::menu_help_about));

	connect_widgets();
	
	update_state();

	_canvas->show();

	_main_window->present();

	_update_pane_position = false;
	_main_paned->set_position(max_pane_position());
	
	_user_pane_position = max_pane_position() - _main_window->get_height()/8;

	_messages_expander->set_expanded(false);
	_pane_closed = true;
	
	_main_paned->property_position().signal_changed().connect(
		sigc::mem_fun(*this, &Patchage::on_pane_position_changed));
	
	_messages_expander->property_expanded().signal_changed().connect(
		sigc::mem_fun(*this, &Patchage::on_messages_expander_changed));

	// Idle callback, check if we need to refresh
	Glib::signal_timeout().connect(sigc::mem_fun(this, &Patchage::idle_callback), 100);
	
	// Faster idle callback to update DSP load progress bar
	//Glib::signal_timeout().connect(sigc::mem_fun(this, &Patchage::update_load), 50);
	
	_update_pane_position = true;
}


Patchage::~Patchage() 
{
	delete _jack_driver;
#ifdef HAVE_ALSA
	delete _alsa_driver;
#endif
#ifdef HAVE_LASH
	delete _lash_driver;
#endif
	delete _state_manager;
}


void
Patchage::attach()
{
	_jack_driver->attach(true);

#ifdef HAVE_LASH
	_lash_driver->attach(true);
#endif
#ifdef HAVE_ALSA
	_alsa_driver->attach();
#endif

	menu_view_refresh();

	update_toolbar();

	//m_status_bar->push("Connected to JACK server");
}
	

bool
Patchage::idle_callback() 
{
	// Process any JACK events
	if (_jack_driver) {
		while (!_jack_driver->events().empty()) {
			PatchageEvent& ev = _jack_driver->events().front();
			_jack_driver->events().pop();
			ev.execute();
		}
	}
	
	// Process any ALSA events
#ifdef HAVE_ALSA
	if (_alsa_driver) {
		while (!_alsa_driver->events().empty()) {
			PatchageEvent& ev = _alsa_driver->events().front();
			_alsa_driver->events().pop();
			ev.execute();
		}
	}
#endif

	// Do a full refresh (ie user clicked refresh)
	if (_refresh) {
		_canvas->flag_all_connections();
		_jack_driver->refresh();
#ifdef HAVE_ALSA
		if (_alsa_driver)
			_alsa_driver->refresh();
#endif
	}

	// Jack driver needs refreshing
	if (_refresh || _jack_driver->is_dirty()) {
		_canvas->flag_all_connections();
		_jack_driver->refresh();
		_canvas->destroy_all_flagged_connections();
		_refresh = false;
	}

#ifdef HAVE_LASH
	if (_lash_driver->is_attached())
		_lash_driver->process_events();
#endif

	if (_refresh)
		_refresh = false;

	update_load();

	return true;
}


void
Patchage::update_toolbar()
{
	//_jack_connect_toggle->set_active(_jack_driver->is_attached());
	//_jack_realtime_check->set_active(_jack_driver->is_realtime());

	if (_jack_driver->is_attached()) {
		_buffer_size_combo->set_active((int)log2f(_jack_driver->buffer_size()) - 5);

		/*switch ((int)m_jack_driver->sample_rate()) {
			case 44100:
				_sample_rate_combo->set_active(0);
				break;
			case 48000:
				_sample_rate_combo->set_active(1);
				break;
			case 96000:
				_sample_rate_combo->set_active(2);
				break;
			default:
				_sample_rate_combo->set_active(-1);
				status_message("[JACK] ERROR: Unknown sample rate");
				break;
		}*/
		stringstream srate;
		srate << _jack_driver->sample_rate()/1000.0;
		_sample_rate_label->set_text(srate.str());
	}
}


bool
Patchage::update_load()
{
	if (!_jack_driver->is_attached())
		return true;

	static float last_delay = 0;

	const float max_delay = _jack_driver->max_delay();

	if (max_delay != last_delay) {
		const float sample_rate = _jack_driver->sample_rate();
		const float buffer_size = _jack_driver->buffer_size();
		const float period      = buffer_size / sample_rate * 1000000; // usecs
		/*
		   if (max_delay > 0) {
		   cerr << "SR: " << sample_rate << ", BS: " << buffer_size << ", P = " << period
		   << ", MD: " << max_delay << endl;
		   }*/

		_xrun_progress_bar->set_fraction(max_delay / period);

		char tmp_buf[8];
		snprintf(tmp_buf, 8, "%zd", _jack_driver->xruns());

		_xrun_progress_bar->set_text(string(tmp_buf) + " Dropouts");

		if (max_delay > period) {
			_xrun_progress_bar->set_fraction(1.0);
			_jack_driver->reset_delay();
		}

		last_delay = max_delay;
	}
	
	return true;
}


void
Patchage::zoom(double z)
{
	_state_manager->set_zoom(z);
	_canvas->set_zoom(z);
}


void
Patchage::zoom_changed() 
{
	static bool enable_signal = true;
	if (enable_signal) {
		enable_signal = false;
		zoom(_zoom_slider->get_value());
		enable_signal = true;
	}
}


void
Patchage::update_state()
{
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		SharedPtr<Module> module = PtrCast<Module>(*i);
		if (module) 
			module->load_location();
	}

	//cerr << "[Patchage] Resizing window: (" << _state_manager->get_window_size().x
	//	<< "," << _state_manager->get_window_size().y << ")" << endl;

	_main_window->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));
	
	//cerr << "[Patchage] Moving window: (" << _state_manager->get_window_location().x
	//	<< "," << _state_manager->get_window_location().y << ")" << endl;
	
	_main_window->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));
}


void
Patchage::status_message(const string& msg) 
{
	if (_status_text->get_buffer()->size() > 0)
		_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), "\n");

	_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), msg);
	_status_text->scroll_to_mark(_status_text->get_buffer()->get_insert(), 0);
}


/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Patchage is already connected to Jack)
 */
void
Patchage::connect_widgets()
{
#ifdef HAVE_LASH
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_lash_connect, &Gtk::MenuItem::set_sensitive), false));
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_lash_connect, &Gtk::MenuItem::set_sensitive), true));
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif

	_jack_driver->signal_attached.connect(
			sigc::mem_fun(this, &Patchage::update_toolbar));

	//_jack_driver->signal_attached.connect(sigc::bind(
	///		sigc::mem_fun(_jack_connect_toggle, &Gtk::ToggleButton::set_active), true));

	_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_jack_connect, &Gtk::MenuItem::set_sensitive), false));
	_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	//_jack_driver->signal_detached.connect(sigc::bind(
	//		sigc::mem_fun(_jack_connect_toggle, &Gtk::ToggleButton::set_active), false));
	_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_jack_connect, &Gtk::MenuItem::set_sensitive), true));
	_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), false));

#ifdef HAVE_ALSA	
	_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), false));
	_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), true));
	_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif
}


#ifdef HAVE_LASH
void
Patchage::menu_open_session() 
{
	Gtk::FileChooserDialog dialog(*_main_window, "Open LASH Session",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		_lash_driver->restore_project(dialog.get_filename());
	}
}


void
Patchage::menu_save_session() 
{
	if (_lash_driver)
		_lash_driver->save_project();
}


void
Patchage::menu_save_session_as() 
{
	if (!_lash_driver)
		return;

	Gtk::FileChooserDialog dialog(*_main_window, "Save LASH Session",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);	
	
	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		_lash_driver->set_project_directory(dialog.get_filename());
		_lash_driver->save_project();
	}
}

	
void
Patchage::menu_close_session() 
{
	cerr << "CLOSE SESSION\n";
	_lash_driver->close_project();
}


void
Patchage::menu_lash_connect() 
{
	_lash_driver->attach(true);
}


void
Patchage::menu_lash_disconnect() 
{
	_lash_driver->detach();
}
#endif

#ifdef HAVE_ALSA
void
Patchage::menu_alsa_connect() 
{
	_alsa_driver->attach(false);
}


void
Patchage::menu_alsa_disconnect() 
{
	_alsa_driver->detach();
	menu_view_refresh();
}
#endif

void
Patchage::menu_store_positions() 
{
	store_window_location();
	_state_manager->save(_settings_filename);
}


void
Patchage::menu_file_quit() 
{
#ifdef HAVE_ALSA
	_alsa_driver->detach();
#endif
	_jack_driver->detach();
	_main_window->hide();
}


void
Patchage::on_pane_position_changed()
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
Patchage::on_messages_expander_changed()
{
	if (!_update_pane_position)
		return;

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
Patchage::show_messages_toggled()
{
	if (_update_pane_position)
		_messages_expander->set_expanded(_menu_view_messages->get_active());
}


void
Patchage::menu_view_refresh() 
{
	assert(_canvas);
	
	_canvas->destroy();
	
	if (_jack_driver)
		_jack_driver->refresh();

#ifdef HAVE_ALSA
	if (_alsa_driver)
		_alsa_driver->refresh();
#endif
}


void
Patchage::menu_view_arrange() 
{
	assert(_canvas);
	
	_canvas->arrange();
}


void
Patchage::view_toolbar_toggled() 
{
	_update_pane_position = false;

	if (_menu_view_toolbar->get_active())
		_toolbar->show();
	else
		_toolbar->hide();
	
	_update_pane_position = true;
}


void
Patchage::menu_help_about() 
{
	_about_window->show();
}


/** Update the stored window location and size in the StateManager (in memory).
 */
void
Patchage::store_window_location()
{
	int loc_x, loc_y, size_x, size_y;
	_main_window->get_position(loc_x, loc_y);
	_main_window->get_size(size_x, size_y);
	Coord window_location;
	window_location.x = loc_x;
	window_location.y = loc_y;
	Coord window_size;
	window_size.x = size_x;
	window_size.y = size_y;
	_state_manager->set_window_location(window_location);
	_state_manager->set_window_size(window_size);
}


void
Patchage::clear_load()
{
	_xrun_progress_bar->set_fraction(0.0);
	_jack_driver->reset_xruns();
	_jack_driver->reset_delay();
}


void
Patchage::buffer_size_changed()
{
	const int selected = _buffer_size_combo->get_active_row_number();

	if (selected == -1) {
		update_toolbar();
	} else {
		jack_nframes_t buffer_size = 1 << (selected+5);
	
		//cerr << "BS Changed: " << selected << ": " << buffer_size << endl;

		if ( ! _jack_driver->set_buffer_size(buffer_size))
			update_toolbar(); // reset combo box to actual value
	}
}


/*
void
Patchage::sample_rate_changed()
{
	const int selected = _sample_rate_combo->get_active_row_number();

	if (selected == -1) {
		update_toolbar();
	} else {
		jack_nframes_t rate = 44100; // selected == 0
		if (selected == 1)
			rate = 48000;
		else if (selected == 2)
			rate = 96000;
	
		//cerr << "SR Changed: " << selected << ": " << rate << endl;

		//m_jack_driver->set_sample_rate(rate);
	}
}

void
Patchage::realtime_changed()
{
	_jack_driver->set_realtime(_jack_realtime_check->get_active());
}

void
Patchage::jack_connect_changed()
{
	const bool selected = _jack_connect_toggle->get_active();

	if (selected != _jack_driver->is_attached()) {
		if (selected) {
			_jack_driver->attach(true);
		} else {
			_jack_driver->detach();
		}
	}
}
*/
