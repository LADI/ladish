/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

	boost::shared_ptr<PatchageFlowCanvas> canvas() { return m_canvas; }
	
	StateManager* state_manager() { return m_state_manager; }
	Gtk::Window*  window()        { return m_main_window; }
	JackDriver*   jack_driver()   { return m_jack_driver; }
#ifdef HAVE_ALSA
	AlsaDriver*   alsa_driver()   { return m_alsa_driver; }
#endif
#ifdef HAVE_LASH
	LashDriver*   lash_driver()   { return m_lash_driver; }
#endif
	
	void attach();
	void quit() { m_main_window->hide(); }

	void update_state();
	void store_window_location();

	void status_message(const string& msg);
	inline void queue_refresh() { m_refresh = true; }

	int max_pane_position()
	{ return m_main_paned->property_max_position() - m_messages_expander->get_label_widget()->get_height() - 8; }

protected:
	void attach_menu_items();

	void menu_store_positions();
	void menu_file_quit();
	void show_messages_toggled();
	void menu_view_refresh();
	void menu_help_about();
	void zoom(double z);
	void zoom_changed();
	bool idle_callback();

	void on_pane_position_changed();
	void on_messages_expander_changed();

	bool m_pane_closed;
	bool m_update_pane_position;
	int  m_user_pane_position;

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

	boost::shared_ptr<PatchageFlowCanvas> m_canvas;

	JackDriver*         m_jack_driver;
	StateManager*       m_state_manager;

	Gtk::Main* m_gtk_main;

	string m_settings_filename;
	bool   m_refresh;
	
	Gtk::Window*         m_main_window;
	JackSettingsDialog*  m_jack_settings_dialog;
	Gtk::AboutDialog*    m_about_window;
	Gtk::MenuItem*       m_menu_jack_settings;
	Gtk::MenuItem*       m_menu_jack_launch;
	Gtk::MenuItem*       m_menu_jack_connect;
	Gtk::MenuItem*       m_menu_jack_disconnect;
	Gtk::MenuItem*       m_menu_store_positions;
	Gtk::MenuItem*       m_menu_file_quit;
	Gtk::CheckMenuItem*  m_menu_view_messages;
	Gtk::MenuItem*       m_menu_view_refresh;
	Gtk::MenuItem*       m_menu_help_about;
	Gtk::ScrolledWindow* m_canvas_scrolledwindow;
	Gtk::HScale*         m_zoom_slider;
	Gtk::TextView*       m_status_text;
	Gtk::Paned*          m_main_paned;
	Gtk::Expander*       m_messages_expander;
	Gtk::Button*         m_zoom_normal_button;
	Gtk::Button*         m_zoom_full_button;
};

#endif // PATCHAGE_H
