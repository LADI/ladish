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

#ifndef MACHINA_GUI_H
#define MACHINA_GUI_H

//#include "config.h"
#include <string>
#include <raul/SharedPtr.h>
#include <libgnomecanvasmm.h>

using namespace std;

namespace Machina { class Machine; }

class MachinaCanvas;

class MachinaGUI
{
public:
	MachinaGUI(SharedPtr<Machina::Machine> machine/*int argc, char** argv*/);
	~MachinaGUI();

	boost::shared_ptr<MachinaCanvas> canvas() { return _canvas; }
	
	Gtk::Window* window() { return _main_window; }
	
	void attach();
	void quit() { _main_window->hide(); }

	void status_message(const string& msg);
	inline void queue_refresh() { _refresh = true; }

	int max_pane_position()
	{ return _main_paned->property_max_position() - _messages_expander->get_label_widget()->get_height() - 8; }

protected:
	void connect_widgets();

	void menu_file_quit();
	void show_messages_toggled();
	void menu_view_refresh();
	void menu_help_about();
	void zoom(double z);
	void zoom_changed();
	bool idle_callback();

	void on_pane_position_changed();
	void on_messages_expander_changed();

	bool _pane_closed;
	bool _update_pane_position;
	int  _user_pane_position;
	
	bool   _refresh;

	boost::shared_ptr<MachinaCanvas> _canvas;
	boost::shared_ptr<Machina::Machine>       _machine;

	Gtk::Main* _gtk_main;
	
	Gtk::Window*         _main_window;
	Gtk::AboutDialog*    _about_window;
	Gtk::MenuItem*       _menu_file_quit;
	Gtk::CheckMenuItem*  _menu_view_messages;
	Gtk::MenuItem*       _menu_view_refresh;
	Gtk::MenuItem*       _menu_help_about;
	Gtk::ScrolledWindow* _canvas_scrolledwindow;
	Gtk::TextView*       _status_text;
	Gtk::Paned*          _main_paned;
	Gtk::Expander*       _messages_expander;
	Gtk::ToolButton*     _zoom_normal_button;
	Gtk::ToolButton*     _zoom_full_button;
};

#endif // MACHINA_GUI_H
