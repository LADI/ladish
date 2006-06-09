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

#ifndef PATCHAGE_H
#define PATCHAGE_H

#include "config.h"
#include <string>
#include <libgnomecanvasmm.h>

using namespace std;

class PatchageFlowCanvas;
class JackDriver;
class AlsaDriver;
class LashDriver;
class StateManager;


class Patchage
{
public:
	Patchage(int argc, char** argv);
	~Patchage();

	PatchageFlowCanvas*   canvas()          { return m_canvas; }
	StateManager*         state_manager()   { return m_state_manager; }
	Gtk::Window*          window()          { return m_main_window; }
	JackDriver*           jack_driver()     { return m_jack_driver; }
#ifdef HAVE_ALSA
	AlsaDriver*           alsa_driver()     { return m_alsa_driver; }
#endif
#ifdef HAVE_LASH
	LashDriver*       lash_driver()     { return m_lash_driver; }
#endif
	
	void attach();
	void quit() { m_main_window->hide(); }

	void update_state();
	void store_window_location();

	void status_message(const string& msg);
	inline void queue_refresh() { m_refresh = true; }

protected:
	void update_menu_items();

	void menu_jack_launch();
	void menu_jack_connect();
	void menu_jack_disconnect();
	void menu_file_save();
	void menu_file_quit();
	void menu_view_refresh();
	void menu_help_about();
	void close_about();
	void zoom_changed();
	bool idle_callback();

#ifdef HAVE_LASH
	LashDriver*    m_lash_driver;
	Gtk::MenuItem* m_menu_lash_launch;
	Gtk::MenuItem* m_menu_lash_connect;
	Gtk::MenuItem* m_menu_lash_disconnect;
	void menu_lash_launch();
	void menu_lash_connect();
	void menu_lash_disconnect();
#endif

#ifdef HAVE_ALSA
	AlsaDriver*    m_alsa_driver;
	Gtk::MenuItem* m_menu_alsa_connect;
	Gtk::MenuItem* m_menu_alsa_disconnect;
	void menu_alsa_connect();
	void menu_alsa_disconnect();
#endif

	PatchageFlowCanvas* m_canvas;
	JackDriver*         m_jack_driver;
	StateManager*       m_state_manager;

	Gtk::Main* m_gtk_main;

	string m_settings_filename;
	bool   m_refresh;
	
	Gtk::Window*         m_main_window;
	Gtk::Window*         m_about_window;
	Gtk::Label*          m_about_project_label;
	Gtk::MenuItem*       m_menu_jack_launch;
	Gtk::MenuItem*       m_menu_jack_connect;
	Gtk::MenuItem*       m_menu_jack_disconnect;
	Gtk::MenuItem*       m_menu_file_save;
	Gtk::MenuItem*       m_menu_file_quit;
	Gtk::MenuItem*       m_menu_view_refresh;
	Gtk::MenuItem*       m_menu_help_about;
	Gtk::ScrolledWindow* m_canvas_scrolledwindow;
	Gtk::HScale*         m_zoom_slider;
	Gtk::Button*         m_about_close_button;
	Gtk::Label*          m_status_label;
};

#endif // PATCHAGE_H
