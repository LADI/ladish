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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGE_H
#define PATCHAGE_H

#include "config.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>

using namespace std;

class PatchageFlowCanvas;
class JackDriver;
class AlsaDriver;
class LashDriver;
class StateManager;
class JackSettingsDialog;


class Patchage
{
public:
	Patchage(int argc, char** argv);
	~Patchage();

	boost::shared_ptr<PatchageFlowCanvas> canvas() { return _canvas; }
	
	StateManager* state_manager() { return _state_manager; }
	Gtk::Window*  window()        { return _main_window; }
	JackDriver*   jack_driver()   { return _jack_driver; }
#ifdef HAVE_ALSA
	AlsaDriver*   alsa_driver()   { return _alsa_driver; }
#endif
#ifdef HAVE_LASH
	LashDriver*   lash_driver()   { return _lash_driver; }
#endif
	
	void attach();
	void quit() { _main_window->hide(); }

	void clear_load();

	void update_state();
	void store_window_location();

	void status_message(const string& msg);
	inline void queue_refresh() { _refresh = true; }

	int max_pane_position()
	{ return _main_paned->property_max_position()
		- _messages_expander->get_label_widget()->get_height() - 10; }

protected:
	void connect_widgets();

	void menu_store_positions();
	void menu_file_quit();
	void show_messages_toggled();
	void view_toolbar_toggled();
	void menu_view_refresh();
	void menu_help_about();
	void zoom(double z);
	void zoom_changed();
	bool idle_callback();
	bool update_load();
	void update_toolbar();

	//void jack_connect_changed();
	void buffer_size_changed();
	//void sample_rate_changed();
	//void realtime_changed();
	
	void on_pane_position_changed();
	void on_messages_expander_changed();

	bool _pane_closed;
	bool _update_pane_position;
	int  _user_pane_position;

#ifdef HAVE_LASH
	LashDriver*    _lash_driver;
	Gtk::MenuItem* _menu_open_session;
	Gtk::MenuItem* _menu_save_session;
	Gtk::MenuItem* _menu_save_session_as;
	Gtk::MenuItem* _menu_close_session;
	Gtk::MenuItem* _menu_lash_launch;
	Gtk::MenuItem* _menu_lash_connect;
	Gtk::MenuItem* _menu_lash_disconnect;
	void menu_open_session();
	void menu_save_session();
	void menu_save_session_as();
	void menu_close_session();
	void menu_lash_launch();
	void menu_lash_connect();
	void menu_lash_disconnect();
#endif

#ifdef HAVE_ALSA
	AlsaDriver*    _alsa_driver;
	Gtk::MenuItem* _menu_alsa_connect;
	Gtk::MenuItem* _menu_alsa_disconnect;
	void menu_alsa_connect();
	void menu_alsa_disconnect();
#endif

	boost::shared_ptr<PatchageFlowCanvas> _canvas;

	JackDriver*         _jack_driver;
	StateManager*       _state_manager;

	Gtk::Main* _gtk_main;

	string _settings_filename;
	bool   _refresh;
	
	Gtk::Window*         _main_window;
	JackSettingsDialog*  _jack_settings_dialog;
	Gtk::AboutDialog*    _about_window;
	Gtk::MenuItem*       _menu_jack_settings;
	Gtk::MenuItem*       _menu_jack_launch;
	Gtk::MenuItem*       _menu_jack_connect;
	Gtk::MenuItem*       _menu_jack_disconnect;
	Gtk::MenuItem*       _menu_store_positions;
	Gtk::MenuItem*       _menu_file_quit;
	Gtk::CheckMenuItem*  _menu_view_toolbar;
	Gtk::CheckMenuItem*  _menu_view_messages;
	Gtk::MenuItem*       _menu_view_refresh;
	Gtk::MenuItem*       _menu_help_about;
	Gtk::Toolbar*        _toolbar;
	Gtk::ScrolledWindow* _canvas_scrolledwindow;
	Gtk::HScale*         _zoom_slider;
	Gtk::TextView*       _status_text;
	Gtk::Paned*          _main_paned;
	Gtk::Expander*       _messages_expander;
	Gtk::Button*         _rewind_button;
	Gtk::Button*         _play_button;
	Gtk::Button*         _stop_button;
	Gtk::ToolButton*     _zoom_normal_button;
	Gtk::ToolButton*     _zoom_full_button;
	//Gtk::ProgressBar*    _load_progress_bar;
	Gtk::ComboBox*       _buffer_size_combo;
	Gtk::Label*          _sample_rate_label;
	Gtk::ProgressBar*    _xrun_progress_bar;
	Gtk::ToolButton*     _clear_load_button;
	//Gtk::Statusbar*      _status_bar;
};

#endif // PATCHAGE_H
