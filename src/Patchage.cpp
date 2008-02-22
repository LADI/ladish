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
#include <fstream>
#include <pthread.h>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <gtk/gtkwindow.h>
#include <jack/statistics.h>
#include <raul/SharedPtr.hpp>

#include CONFIG_H_PATH
#include "GladeFile.hpp"
#include "JackDriver.hpp"
#include "JackSettingsDialog.hpp"
#include "Patchage.hpp"
#include "PatchageCanvas.hpp"
#include "PatchageEvent.hpp"
#include "StateManager.hpp"
#ifdef HAVE_ALSA
#include "AlsaDriver.hpp"
#endif
#ifdef HAVE_LASH
#include "LashDriver.hpp"
#endif

using namespace std;

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


#define INIT_WIDGET(x) x(xml, ((const char*)#x) + 1)

Patchage::Patchage(int argc, char** argv)
	: xml(GladeFile::open("patchage"))
#ifdef HAVE_LASH
	, _lash_driver(NULL)
	, INIT_WIDGET(_menu_open_session)
	, INIT_WIDGET(_menu_save_session)
	, INIT_WIDGET(_menu_save_session_as)
	, INIT_WIDGET(_menu_close_session)
	, INIT_WIDGET(_menu_lash_connect)
	, INIT_WIDGET(_menu_lash_disconnect)
#endif
#ifdef HAVE_ALSA
	, _alsa_driver(NULL)
	, _alsa_driver_autoattach(true)
	, INIT_WIDGET(_menu_alsa_connect)
	, INIT_WIDGET(_menu_alsa_disconnect)
#endif
	, _jack_driver(NULL)
	, _state_manager(NULL)
	, _attach(true)
	, _refresh(false)
	, _enable_refresh(true)
	, _jack_settings_dialog(NULL)
	, INIT_WIDGET(_about_win)
	, INIT_WIDGET(_buffer_size_combo)
	, INIT_WIDGET(_clear_load_but)
	, INIT_WIDGET(_main_scrolledwin)
	, INIT_WIDGET(_main_win)
	, INIT_WIDGET(_main_xrun_progress)
	, INIT_WIDGET(_menu_file_quit)
	, INIT_WIDGET(_menu_help_about)
	, INIT_WIDGET(_menu_jack_connect)
	, INIT_WIDGET(_menu_jack_disconnect)
	, INIT_WIDGET(_menu_jack_settings)
	, INIT_WIDGET(_menu_store_positions)
	, INIT_WIDGET(_menu_view_arrange)
	, INIT_WIDGET(_menu_view_messages)
	, INIT_WIDGET(_menu_view_refresh)
	, INIT_WIDGET(_menu_view_toolbar)
	, INIT_WIDGET(_messages_win)
	, INIT_WIDGET(_messages_clear_but)
	, INIT_WIDGET(_messages_close_but)
	, INIT_WIDGET(_play_but)
	, INIT_WIDGET(_rewind_but)
	, INIT_WIDGET(_sample_rate_label)
	, INIT_WIDGET(_status_text)
	, INIT_WIDGET(_stop_but)
	, INIT_WIDGET(_toolbar)
	, INIT_WIDGET(_toolbars_box)
	, INIT_WIDGET(_zoom_full_but)
	, INIT_WIDGET(_zoom_normal_but)
{
	_settings_filename = getenv("HOME");
	_settings_filename += "/.patchagerc";
	_state_manager = new StateManager();
	_canvas = boost::shared_ptr<PatchageCanvas>(new PatchageCanvas(this, 1600*2, 1200*2));

	while (argc > 0) {
		if (!strcmp(*argv, "--help")) {
			cout << "Usage: patchage [OPTIONS]\nOptions: --no-alsa" << endl;
			exit(0);
#ifdef HAVE_ALSA
		} else if (!strcmp(*argv, "-A") || !strcmp(*argv, "--no-alsa")) {
			_alsa_driver_autoattach = false;
#endif
		}

		argv++;
		argc--;
	}

	xml->get_widget_derived("jack_settings_win", _jack_settings_dialog);
	
	Glib::set_application_name("Patchage");
	_about_win->property_program_name() = "Patchage";
	_about_win->property_logo_icon_name() = "patchage";
	gtk_window_set_default_icon_name("patchage");
	
	gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);

	_main_scrolledwin->add(*_canvas);
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                   static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	_main_scrolledwin->property_hadjustment().get_value()->set_step_increment(10);
	_main_scrolledwin->property_vadjustment().get_value()->set_step_increment(10);

	_buffer_size_combo->signal_changed().connect(
			sigc::mem_fun(this, &Patchage::buffer_size_changed));
	_rewind_but->signal_clicked().connect(
			sigc::mem_fun(_jack_driver, &JackDriver::rewind_transport));
	_play_but->signal_clicked().connect(
			sigc::mem_fun(_jack_driver, &JackDriver::start_transport));
	_stop_but->signal_clicked().connect(
			sigc::mem_fun(_jack_driver, &JackDriver::stop_transport));
	_clear_load_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::clear_load));
	_zoom_normal_but->signal_clicked().connect(sigc::bind(
			sigc::mem_fun(this, &Patchage::zoom), 1.0));
	_zoom_full_but->signal_clicked().connect(
			sigc::mem_fun(_canvas.get(), &PatchageCanvas::zoom_full));
	_menu_jack_settings->signal_activate().connect(sigc::hide_return(
			sigc::mem_fun(_jack_settings_dialog, &JackSettingsDialog::run)));
	_menu_jack_connect->signal_activate().connect(sigc::bind(
			sigc::mem_fun(_jack_driver, &JackDriver::attach), true));
	_menu_jack_disconnect->signal_activate().connect(
			sigc::mem_fun(_jack_driver, &JackDriver::detach));

#ifdef HAVE_LASH
	_menu_open_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_open_session));
	_menu_save_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_save_session));
	_menu_save_session_as->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_save_session_as));
	_menu_close_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_close_session));
	_menu_lash_connect->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_lash_connect));
	_menu_lash_disconnect->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_lash_disconnect));
#endif

#ifdef HAVE_ALSA
	_menu_alsa_connect->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_alsa_connect));
	_menu_alsa_disconnect->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_alsa_disconnect));
#endif
	
	_menu_store_positions->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_store_positions));
	_menu_file_quit->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_quit));
	_menu_view_refresh->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::refresh));
	_menu_view_arrange->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_arrange));
	_menu_view_toolbar->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_view_toolbar));
	_menu_view_messages->signal_toggled().connect(
			sigc::mem_fun(this, &Patchage::on_show_messages));
	_menu_help_about->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_help_about));
	
	_messages_clear_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::on_messages_clear));
	_messages_close_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::on_messages_close));
	_messages_win->signal_delete_event().connect(
			sigc::mem_fun(this, &Patchage::on_messages_delete));

	_canvas->show();

	_main_win->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));

	_main_win->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));
	
	_main_win->present();
	_about_win->set_transient_for(*_main_win);
	
	_jack_driver = new JackDriver(this);
	_jack_driver->signal_detached.connect(sigc::mem_fun(this, &Patchage::queue_refresh));

#ifdef HAVE_ALSA
	_alsa_driver = new AlsaDriver(this);
#endif
	
	_state_manager->load(_settings_filename);

#ifdef HAVE_LASH
	_lash_driver = new LashDriver(this, argc, argv);
#endif
	
	connect_widgets();
	update_state();

	// Idle callback, check if we need to refresh
	Glib::signal_timeout().connect(
			sigc::mem_fun(this, &Patchage::idle_callback), 100);
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
	_enable_refresh = false;

	_jack_driver->attach(true);

#ifdef HAVE_LASH
	_lash_driver->attach(true);
#endif
#ifdef HAVE_ALSA
	if (_alsa_driver_autoattach)
		_alsa_driver->attach();
#endif

	_enable_refresh = true;

	refresh();

	update_toolbar();

	//m_status_bar->push("Connected to JACK server");
}
	

bool
Patchage::idle_callback() 
{
	// Initial run, attach
	if (_attach) {
		attach();
		_attach = false;
	}

	// Process any JACK events
	if (_jack_driver) {
		while (!_jack_driver->events().empty()) {
			PatchageEvent& ev = _jack_driver->events().front();
			_jack_driver->events().pop();
			ev.execute(this);
		}
	}
	
	// Process any ALSA events
#ifdef HAVE_ALSA
	if (_alsa_driver) {
		while (!_alsa_driver->events().empty()) {
			PatchageEvent& ev = _alsa_driver->events().front();
			_alsa_driver->events().pop();
			ev.execute(this);
		}
	}
#endif

	// Do a full refresh (ie user clicked refresh)
	if (_refresh) {
		_canvas->destroy();
		_jack_driver->refresh();
#ifdef HAVE_ALSA
		if (_alsa_driver)
			_alsa_driver->refresh();
#endif
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
	if (_enable_refresh && _jack_driver->is_attached())
		_buffer_size_combo->set_active((int)log2f(_jack_driver->buffer_size()) - 5);
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
		const float period      = buffer_size / sample_rate * 1000000; // usec 
		
		_main_xrun_progress->set_fraction(max_delay / period);

		char tmp_buf[8];
		snprintf(tmp_buf, 8, "%zd", _jack_driver->xruns());

		_main_xrun_progress->set_text(string(tmp_buf) + " Dropouts");

		if (max_delay > period) {
			_main_xrun_progress->set_fraction(1.0);
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
Patchage::refresh() 
{
	assert(_canvas);

	if (_enable_refresh) {

		_canvas->destroy();

		if (_jack_driver)
			_jack_driver->refresh();

#ifdef HAVE_ALSA
		if (_alsa_driver)
			_alsa_driver->refresh();
#endif
	
		for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
			(*i)->resize();
		}
	}
}


/** Update the stored window location and size in the StateManager (in memory).
 */
void
Patchage::store_window_location()
{
	int loc_x, loc_y, size_x, size_y;
	_main_win->get_position(loc_x, loc_y);
	_main_win->get_size(size_x, size_y);
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
	_main_xrun_progress->set_fraction(0.0);
	_jack_driver->reset_xruns();
	_jack_driver->reset_delay();
}


void
Patchage::status_message(const string& msg) 
{
	if (_status_text->get_buffer()->size() > 0)
		_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), "\n");

	_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), msg);
	_status_text->scroll_to_mark(_status_text->get_buffer()->get_insert(), 0);
}


void
Patchage::update_state()
{
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		SharedPtr<Module> module = PtrCast<Module>(*i);
		if (module) 
			module->load_location();
	}
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
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), false));
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), true));
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif

	_jack_driver->signal_attached.connect(
			sigc::mem_fun(this, &Patchage::update_toolbar));
	_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_jack_connect, &Gtk::MenuItem::set_sensitive), false));
	_jack_driver->signal_attached.connect(
			sigc::mem_fun(this, &Patchage::refresh));
	_jack_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_jack_connect, &Gtk::MenuItem::set_sensitive), true));
	_jack_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_jack_disconnect, &Gtk::MenuItem::set_sensitive), false));

#ifdef HAVE_ALSA	
	_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), false));
	_alsa_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_alsa_connect, &Gtk::MenuItem::set_sensitive), true));
	_alsa_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_alsa_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif
}


#ifdef HAVE_LASH
void
Patchage::menu_open_session() 
{
	Gtk::FileChooserDialog dialog(*_main_win, "Open LASH Session",
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

	Gtk::FileChooserDialog dialog(*_main_win, "Save LASH Session",
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
	_alsa_driver->refresh();
}


void
Patchage::menu_alsa_disconnect() 
{
	_alsa_driver->detach();
	refresh();
}
#endif


void
Patchage::on_arrange() 
{
	assert(_canvas);
	
	_canvas->arrange();
}

	
void
Patchage::on_help_about() 
{
	_about_win->run();
	_about_win->hide();
}


void
Patchage::on_messages_clear()
{
	_status_text->get_buffer()->erase(
			_status_text->get_buffer()->begin(),
			_status_text->get_buffer()->end());
}

	
void
Patchage::on_messages_close() 
{
	_menu_view_messages->set_active(false);
}

	
bool
Patchage::on_messages_delete(GdkEventAny*) 
{
	_menu_view_messages->set_active(false);
	return true;
}


void
Patchage::on_quit() 
{
#ifdef HAVE_ALSA
	_alsa_driver->detach();
#endif
	_jack_driver->detach();
	_main_win->hide();
}


void
Patchage::on_show_messages()
{
	if (_menu_view_messages->get_active())
		_messages_win->present();
	else
		_messages_win->hide();
}

	
void
Patchage::on_store_positions() 
{
	store_window_location();
	_state_manager->save(_settings_filename);
}


void
Patchage::on_view_toolbar() 
{
	if (_menu_view_toolbar->get_active())
		_toolbars_box->show();
	else
		_toolbars_box->hide();
}


void
Patchage::buffer_size_changed()
{
	const int selected = _buffer_size_combo->get_active_row_number();

	if (selected == -1) {
		update_toolbar();
	} else {
		jack_nframes_t buffer_size = 1 << (selected+5);
	
		if ( ! _jack_driver->set_buffer_size(buffer_size))
			update_toolbar(); // reset combo box to actual value
	}
}

