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

#include <string>
#include <raul/SharedPtr.h>
#include <raul/Maid.h>
#include <libgnomecanvasmm.h>
#include <machina/Engine.hpp>

using namespace std;

namespace Machina { class Machine; class Engine; }

class MachinaCanvas;

class MachinaGUI
{
public:
	MachinaGUI(SharedPtr<Machina::Engine> engine);
	~MachinaGUI();

	boost::shared_ptr<MachinaCanvas>    canvas()  { return _canvas; }
	boost::shared_ptr<Machina::Machine> machine() { return _engine->machine(); }
	
	SharedPtr<Raul::Maid> maid() { return _maid; }
	
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
	void menu_file_open();
	void menu_file_save();
	void menu_file_save_as();
	void menu_import_midi();
	void menu_export_midi();
	void menu_export_graphviz();
	//void show_messages_toggled();
	void show_toolbar_toggled();
	//void menu_view_refresh();
	void menu_help_about();
	void menu_help_help();
	void zoom(double z);
	void zoom_changed();
	bool idle_callback();
	void update_toolbar();
	bool scrolled_window_event(GdkEvent* ev);

	void on_pane_position_changed();
	void on_messages_expander_changed();
	
	void quantize_changed();
	void tempo_changed();

	bool _pane_closed;
	bool _update_pane_position;
	int  _user_pane_position;
	
	bool _refresh;

	string _save_uri;

	boost::shared_ptr<MachinaCanvas>   _canvas;
	boost::shared_ptr<Machina::Engine> _engine;
	
	SharedPtr<Raul::Maid> _maid;

	Gtk::Main* _gtk_main;
	
	Gtk::Window*         _main_window;
	Gtk::Dialog*         _help_dialog;
	Gtk::AboutDialog*    _about_window;
	Gtk::Toolbar*        _toolbar;
	Gtk::MenuItem*       _menu_file_open;
	Gtk::MenuItem*       _menu_file_save;
	Gtk::MenuItem*       _menu_file_save_as;
	Gtk::MenuItem*       _menu_file_quit;
	Gtk::MenuItem*       _menu_import_midi;
	Gtk::MenuItem*       _menu_export_midi;
	Gtk::MenuItem*       _menu_export_graphviz;
	Gtk::MenuItem*       _menu_help_about;
	Gtk::CheckMenuItem*  _menu_view_toolbar;
	//Gtk::CheckMenuItem*  _menu_view_messages;
	Gtk::MenuItem*       _menu_view_refresh;
	Gtk::MenuItem*       _menu_help_help;
	Gtk::ScrolledWindow* _canvas_scrolledwindow;
	Gtk::TextView*       _status_text;
	Gtk::Paned*          _main_paned;
	Gtk::Expander*       _messages_expander;
	Gtk::RadioButton*    _slave_radiobutton;
	Gtk::RadioButton*    _bpm_radiobutton;
	Gtk::SpinButton*     _bpm_spinbutton;
	Gtk::CheckButton*    _quantize_checkbutton;
	Gtk::SpinButton*     _quantize_spinbutton;
	Gtk::ToolButton*     _zoom_normal_button;
	Gtk::ToolButton*     _zoom_full_button;
	Gtk::ToolButton*     _arrange_button;
};

#endif // MACHINA_GUI_H
