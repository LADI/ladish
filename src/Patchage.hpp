/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "PatchagePort.hpp"

class PatchageCanvas;
class a2j_proxy;
class jack_proxy;
class lash_proxy;
class StateManager;
class project_list;
class session;

class Patchage {
public:
	Patchage(int argc, char** argv);
	~Patchage();

	boost::shared_ptr<PatchageCanvas> canvas() const { return _canvas; }
	
	Gtk::Window* window() { return _main_win.get(); }
	
	StateManager* state_manager() const { return _state_manager; }

	void quit() { _main_win->hide(); }

	void connect(boost::shared_ptr<PatchagePort> p1, boost::shared_ptr<PatchagePort> p2);
	void disconnect(boost::shared_ptr<PatchagePort> p1, boost::shared_ptr<PatchagePort> p2);

	void        refresh();

	void clear_load();
	void info_msg(const std::string& msg);
	void error_msg(const std::string& msg);
	void status_msg(const std::string& msg);
	void update_state();
	void store_window_location();
	
	void set_lash_availability(bool lash_active);
	void set_a2j_status(unsigned int status);

	void load_project_ask();
	void load_project(const std::string& project_name);
	void save_all_projects();
	void save_project(const std::string& project_name);
	void close_project(const std::string& project_name);
	void close_all_projects();

	void
	on_port_added(
		const char * client_name,
		const char * port_name,
		PortType port_type,
		bool is_input,
		bool is_terminal);

	void
	on_port_removed(
		const char * client_name,
		const char * port_name);

	void
	on_ports_connected(
		const char * client1_name,
		const char * port1_name,
		const char * client2_name,
		const char * port2_name);

	void
	on_ports_disconnected(
		const char * client1_name,
		const char * port1_name,
		const char * client2_name,
		const char * port2_name);

	void
	clear_canvas();

	bool
	is_canvas_empty();

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
	void update_load();
	void update_toolbar();

	void jack_status_changed(bool started);

	void buffer_size_changed();

	void
	get_port_jack_names(
		boost::shared_ptr<PatchagePort> port,
		std::string& jack_client_name,
		std::string& jack_port_name);
	
	boost::shared_ptr<PatchagePort>
	lookup_port(
		const char * jack_client_name,
		const char * jack_port_name);

	boost::shared_ptr<PatchageCanvas> _canvas;

	a2j_proxy * _a2j;
	jack_proxy * _jack;
	session * _session;
	lash_proxy * _lash;
	project_list * _project_list;
	StateManager * _state_manager;

	Gtk::Main* _gtk_main;

	std::string _settings_filename;
	float _max_dsp_load;
	
	Widget<Gtk::AboutDialog>    _about_win;
	Widget<Gtk::ComboBox>       _buffer_size_combo;
	Widget<Gtk::ToolButton>     _clear_load_but;
	Widget<Gtk::ScrolledWindow> _main_scrolledwin;
	Widget<Gtk::Window>         _main_win;
	Widget<Gtk::ProgressBar>    _main_xrun_progress;
	Widget<Gtk::Label>          _main_a2j_status_label;
	Widget<Gtk::Label>          _main_lash_status_label;
	Widget<Gtk::MenuItem>       _menu_file_quit;
	Widget<Gtk::MenuItem>       _menu_help_about;
	Widget<Gtk::MenuItem>       _menu_jack_start;
	Widget<Gtk::MenuItem>       _menu_jack_stop;
	Widget<Gtk::MenuItem>       _menu_a2j_start;
	Widget<Gtk::MenuItem>       _menu_a2j_stop;
	Widget<Gtk::MenuItem>       _menu_lash_activate;
	Widget<Gtk::MenuItem>       _menu_lash_deactivate;
	Widget<Gtk::MenuItem>       _menu_load_project;
	Widget<Gtk::MenuItem>       _menu_save_all_projects;
	Widget<Gtk::MenuItem>       _menu_close_all_projects;
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
