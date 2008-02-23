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

#ifndef PATCHAGE_H
#define PATCHAGE_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include CONFIG_H_PATH
#include "Widget.hpp"

class PatchageCanvas;
class JackDriver;
class AlsaDriver;
class LashDriver;
class StateManager;
class JackSettingsDialog;

class Patchage {
public:
	Patchage(int argc, char** argv);
	~Patchage();

	boost::shared_ptr<PatchageCanvas> canvas() const { return _canvas; }
	
	Gtk::Window* window() { return _main_win.get(); }
	
	StateManager* state_manager() const { return _state_manager; }
	JackDriver*   jack_driver()   const { return _jack_driver; }
#ifdef HAVE_ALSA
	AlsaDriver*   alsa_driver()   const { return _alsa_driver; }
#endif
#ifdef HAVE_LASH
	LashDriver*   lash_driver()   const { return _lash_driver; }
#endif
	
	void attach();
	void quit() { _main_win->hide(); }
	
	void        refresh();
	inline void queue_refresh() { _refresh = true; }

	void clear_load();
	void status_message(const std::string& msg);
	void update_state();
	void store_window_location();

protected:
	void connect_widgets();

	void on_arrange();
	void on_help_about();
	void on_messages_clear();
	void on_messages_close();
	bool on_messages_delete(GdkEventAny*);
	void on_quit();
	void on_show_messages();
	void on_store_positions();
	void on_view_toolbar();

	void zoom(double z);
	bool idle_callback();
	bool update_load();
	void update_toolbar();

	void buffer_size_changed();
	
	Glib::RefPtr<Gnome::Glade::Xml> xml;

#ifdef HAVE_LASH
	LashDriver*    _lash_driver;
	Widget<Gtk::MenuItem> _menu_open_session;
	Widget<Gtk::MenuItem> _menu_save_session;
	Widget<Gtk::MenuItem> _menu_save_session_as;
	Widget<Gtk::MenuItem> _menu_close_session;
	Widget<Gtk::MenuItem> _menu_lash_connect;
	Widget<Gtk::MenuItem> _menu_lash_disconnect;
	void menu_open_session();
	void menu_save_session();
	void menu_save_session_as();
	void menu_close_session();
	void menu_lash_connect();
	void menu_lash_disconnect();
#endif

#ifdef HAVE_ALSA
	AlsaDriver*    _alsa_driver;
	bool _alsa_driver_autoattach;
	Widget<Gtk::MenuItem> _menu_alsa_connect;
	Widget<Gtk::MenuItem> _menu_alsa_disconnect;
	void menu_alsa_connect();
	void menu_alsa_disconnect();
#endif

	boost::shared_ptr<PatchageCanvas> _canvas;

	JackDriver*         _jack_driver;
	StateManager*       _state_manager;

	Gtk::Main* _gtk_main;

	std::string _settings_filename;
	bool        _attach;
	bool        _refresh;
	bool        _enable_refresh;
	bool        _pane_closed;
	bool        _update_pane_position;
	int         _user_pane_position;
	
	JackSettingsDialog*  _jack_settings_dialog;
	
	Widget<Gtk::AboutDialog>    _about_win;
	Widget<Gtk::ComboBox>       _buffer_size_combo;
	Widget<Gtk::ToolButton>     _clear_load_but;
	Widget<Gtk::ScrolledWindow> _main_scrolledwin;
	Widget<Gtk::Window>         _main_win;
	Widget<Gtk::ProgressBar>    _main_xrun_progress;
	Widget<Gtk::MenuItem>       _menu_file_quit;
	Widget<Gtk::MenuItem>       _menu_help_about;
	Widget<Gtk::MenuItem>       _menu_jack_connect;
	Widget<Gtk::MenuItem>       _menu_jack_disconnect;
	Widget<Gtk::MenuItem>       _menu_jack_settings;
	Widget<Gtk::MenuItem>       _menu_store_positions;
	Widget<Gtk::MenuItem>       _menu_view_arrange;
	Widget<Gtk::CheckMenuItem>  _menu_view_messages;
	Widget<Gtk::MenuItem>       _menu_view_refresh;
	Widget<Gtk::CheckMenuItem>  _menu_view_toolbar;
	Widget<Gtk::Dialog>         _messages_win;
	Widget<Gtk::Button>         _messages_clear_but;
	Widget<Gtk::Button>         _messages_close_but;
	Widget<Gtk::Label>          _sample_rate_label;
	Widget<Gtk::TextView>       _status_text;
	Widget<Gtk::Toolbar>        _toolbar;
	Widget<Gtk::ToolButton>     _zoom_full_but;
	Widget<Gtk::ToolButton>     _zoom_normal_but;
};

#endif // PATCHAGE_H
