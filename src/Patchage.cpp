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
#include <boost/format.hpp>

#include CONFIG_H_PATH
#include "GladeFile.hpp"
#include "JackDbusDriver.hpp"
#include "JackSettingsDialog.hpp"
#include "Patchage.hpp"
#include "PatchageCanvas.hpp"
#include "StateManager.hpp"

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
	, INIT_WIDGET(_menu_open_session)
	, INIT_WIDGET(_menu_save_session)
	, INIT_WIDGET(_menu_save_session_as)
	, INIT_WIDGET(_menu_close_session)
	, _jack_driver(NULL)
	, _state_manager(NULL)
	, _max_dsp_load(0.0)
	, INIT_WIDGET(_about_win)
	, INIT_WIDGET(_buffer_size_combo)
	, INIT_WIDGET(_clear_load_but)
	, INIT_WIDGET(_main_scrolledwin)
	, INIT_WIDGET(_main_win)
	, INIT_WIDGET(_main_xrun_progress)
	, INIT_WIDGET(_menu_file_quit)
	, INIT_WIDGET(_menu_help_about)
	, INIT_WIDGET(_menu_jack_start)
	, INIT_WIDGET(_menu_jack_stop)
	, INIT_WIDGET(_menu_store_positions)
	, INIT_WIDGET(_menu_view_arrange)
	, INIT_WIDGET(_menu_view_messages)
	, INIT_WIDGET(_menu_view_refresh)
	, INIT_WIDGET(_menu_view_toolbar)
	, INIT_WIDGET(_messages_win)
	, INIT_WIDGET(_messages_clear_but)
	, INIT_WIDGET(_messages_close_but)
	, INIT_WIDGET(_sample_rate_label)
	, INIT_WIDGET(_status_text)
	, INIT_WIDGET(_toolbar)
	, INIT_WIDGET(_zoom_full_but)
	, INIT_WIDGET(_zoom_normal_but)
	, INIT_WIDGET(_projects_list)
{
	_settings_filename = getenv("HOME");
	_settings_filename += "/.patchagerc";
	_state_manager = new StateManager();
	_canvas = boost::shared_ptr<PatchageCanvas>(new PatchageCanvas(this, 1600*2, 1200*2));

	while (argc > 0) {
		if (!strcmp(*argv, "--help")) {
			cout << "Usage: patchage [OPTIONS]\nOptions: --no-alsa" << endl;
			exit(0);
		}

		argv++;
		argc--;
	}

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

	_main_scrolledwin->signal_scroll_event().connect(
			sigc::mem_fun(this, &Patchage::on_scroll));

	_buffer_size_combo->signal_changed().connect(
			sigc::mem_fun(this, &Patchage::buffer_size_changed));
	_clear_load_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::clear_load));
	_zoom_normal_but->signal_clicked().connect(sigc::bind(
			sigc::mem_fun(this, &Patchage::zoom), 1.0));
	_zoom_full_but->signal_clicked().connect(
			sigc::mem_fun(_canvas.get(), &PatchageCanvas::zoom_full));

	_menu_open_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_open_session));
	_menu_save_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_save_session));
	_menu_save_session_as->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_save_session_as));
	_menu_close_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::menu_close_session));

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
	_main_win->present();
	
	_state_manager->load(_settings_filename);
	
	_main_win->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));

	_main_win->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));
	
	_about_win->set_transient_for(*_main_win);
	
	_jack_driver = new JackDriver(this);

 	_menu_jack_start->signal_activate().connect(
 		sigc::mem_fun(_jack_driver, &JackDriver::start_server));
 	_menu_jack_stop->signal_activate().connect(
 		sigc::mem_fun(_jack_driver, &JackDriver::stop_server));

	jack_status_changed(_jack_driver->is_started());

	connect_widgets();
	update_state();

	_canvas->grab_focus();

	// Idle callback, check if we need to refresh
	Glib::signal_timeout().connect(
			sigc::mem_fun(this, &Patchage::idle_callback), 100);
}

Patchage::~Patchage() 
{
	delete _jack_driver;
	delete _state_manager;

	_about_win.destroy();
	_messages_win.destroy();
	_main_win.destroy();
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
	bool started;

	started = _jack_driver->is_started();

	_buffer_size_combo->set_sensitive(started);

	if (started)
	{
		_buffer_size_combo->set_active((int)log2f(_jack_driver->buffer_size()) - 5);
	}
}


void
Patchage::update_load()
{
	if (!_jack_driver->is_started())
	{
		_main_xrun_progress->set_text("JACK stopped");
		return;
	}

	char tmp_buf[8];
	snprintf(tmp_buf, 8, "%zd", _jack_driver->xruns());

	_main_xrun_progress->set_text(string(tmp_buf) + " Dropouts");

	float load = _jack_driver->get_dsp_load();

	load /= 100.0;								// dbus returns it in percents, we use 0..1

	if (load > _max_dsp_load)
	{
		_max_dsp_load = load;
		_main_xrun_progress->set_fraction(load);
	}
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

	_canvas->destroy();

	if (_jack_driver)
		_jack_driver->refresh();

	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		(*i)->resize();
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
	_max_dsp_load = 0.0;
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
#if 0
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), false));
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), true));
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif

	_jack_driver->signal_started.connect(
		sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), true));
	
	_jack_driver->signal_stopped.connect(
		sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), false));
}

void
Patchage::jack_status_changed(
	bool started)
{
	update_toolbar();

	_menu_jack_start->set_sensitive(!started);
	_menu_jack_stop->set_sensitive(started);
	_clear_load_but->set_sensitive(started);
}

void
Patchage::menu_open_session() 
{
#if 0
	Gtk::FileChooserDialog dialog(*_main_win, "Open LASH Session",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		_lash_driver->restore_project(dialog.get_filename());
	}
#endif
}


void
Patchage::menu_save_session() 
{
#if 0
	if (_lash_driver)
		_lash_driver->save_project();
#endif
}


void
Patchage::menu_save_session_as() 
{
#if 0
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
#endif
}

	
void
Patchage::menu_close_session() 
{
	//_lash_driver->close_project();
}

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
		_toolbar->show();
	else
		_toolbar->hide();
}

	
bool
Patchage::on_scroll(GdkEventScroll* ev) 
{
	cout << "ON SCROLL" << endl;
	return false;
}


void
Patchage::buffer_size_changed()
{
	const int selected = _buffer_size_combo->get_active_row_number();

	if (selected == -1) {
		update_toolbar();
	} else {
		jack_nframes_t buffer_size = 1 << (selected+5);
	
		// this check is temporal workaround for jack bug
		// we skip setting buffer size if it same as acutal one
		// proper place for such check is in jack
		if (_jack_driver->buffer_size() != buffer_size)
		{
			if (!_jack_driver->set_buffer_size(buffer_size))
			{
				update_toolbar(); // reset combo box to actual value
			}
		}
	}
}

