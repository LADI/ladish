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

#ifndef PATCHAGE_PATCHAGE_HPP
#define PATCHAGE_PATCHAGE_HPP

#include <string>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include CONFIG_H_PATH
#include "Widget.hpp"

class PatchageCanvas;
class JackDriver;
class AlsaDriver;
class LashProxy;
class StateManager;
class JackSettingsDialog;
class Session;
class DBus;
class ProjectList;

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
#ifdef HAVE_DBUS
	LashProxy*   lash_proxy()     const { return _lash_proxy; }
	DBus*        dbus()           const { return _dbus; }
	
	void show_load_project_dialog();
	void set_lash_available(bool available);
#endif
	
	Glib::RefPtr<Gnome::Glade::Xml> xml() { return _xml; }
	
	void attach();
	void quit() { _main_win->hide(); }
	
	void        refresh();
	inline void queue_refresh() { _refresh = true; }

	void clear_load();
	void info_msg(const std::string& msg);
	void error_msg(const std::string& msg);
	void status_msg(const std::string& msg);
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
	void on_show_projects();
	void on_store_positions();
	void on_view_toolbar();
	bool on_scroll(GdkEventScroll* ev);

	void zoom(double z);
	bool idle_callback();
	bool update_load();
	void update_toolbar();

	void buffer_size_changed();
	
	Glib::RefPtr<Gnome::Glade::Xml> _xml;

#ifdef HAVE_DBUS
	LashProxy*   _lash_proxy;
	DBus*        _dbus;
	ProjectList* _project_list;
	Session*     _session;
#endif

#ifdef HAVE_ALSA
	AlsaDriver* _alsa_driver;
	bool _alsa_driver_autoattach;
	void menu_alsa_connect();
	void menu_alsa_disconnect();
#endif

	boost::shared_ptr<PatchageCanvas> _canvas;

	JackDriver*   _jack_driver;
	StateManager* _state_manager;

	Gtk::Main* _gtk_main;

	std::string _settings_filename;
	bool        _attach;
	bool        _refresh;
	bool        _enable_refresh;
	
	Widget<Gtk::AboutDialog>    _about_win;
	Widget<Gtk::ComboBox>       _buffer_size_combo;
	Widget<Gtk::ToolButton>     _clear_load_but;
	Widget<Gtk::ScrolledWindow> _main_scrolledwin;
	Widget<Gtk::Window>         _main_win;
	Widget<Gtk::ProgressBar>    _main_xrun_progress;
	Widget<Gtk::MenuItem>       _menu_alsa_connect;
	Widget<Gtk::MenuItem>       _menu_alsa_disconnect;
	Widget<Gtk::MenuItem>       _menu_file_quit;
	Widget<Gtk::MenuItem>       _menu_help_about;
	Widget<Gtk::MenuItem>       _menu_jack_connect;
	Widget<Gtk::MenuItem>       _menu_jack_disconnect;
	Widget<Gtk::MenuItem>       _menu_open_session;
	Widget<Gtk::MenuItem>       _menu_store_positions;
	Widget<Gtk::MenuItem>       _menu_view_arrange;
	Widget<Gtk::CheckMenuItem>  _menu_view_messages;
	Widget<Gtk::CheckMenuItem>  _menu_view_projects;
	Widget<Gtk::MenuItem>       _menu_view_refresh;
	Widget<Gtk::CheckMenuItem>  _menu_view_toolbar;
	Widget<Gtk::Button>         _messages_clear_but;
	Widget<Gtk::Button>         _messages_close_but;
	Widget<Gtk::Dialog>         _messages_win;
	Widget<Gtk::Viewport>       _project_list_viewport;
	Widget<Gtk::Label>          _sample_rate_label;
	Widget<Gtk::TextView>       _status_text;
	Widget<Gtk::Toolbar>        _toolbar;
	Widget<Gtk::ToolButton>     _zoom_full_but;
	Widget<Gtk::ToolButton>     _zoom_normal_but;
};

#endif // PATCHAGE_PATCHAGE_HPP
