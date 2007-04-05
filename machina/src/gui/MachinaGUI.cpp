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

#include <cmath>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <pthread.h>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <raul/RDFWriter.h>
#include <machina/Machine.hpp>
#include <machina/SMFDriver.hpp>
#include "GladeXml.hpp"
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"
#include "EdgeView.hpp"


MachinaGUI::MachinaGUI(SharedPtr<Machina::Engine> engine)
: _pane_closed(false),
  _update_pane_position(true),
  _user_pane_position(0),
  _refresh(false),
  _engine(engine),
  _maid(new Raul::Maid(32))
{
	/*_settings_filename = getenv("HOME");
	_settings_filename += "/.machinarc";*/

	_canvas = boost::shared_ptr<MachinaCanvas>(new MachinaCanvas(this, 1600*2, 1200*2));

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeXml::create();

	xml->get_widget("machina_win", _main_window);
	xml->get_widget("about_win", _about_window);
	xml->get_widget("help_dialog", _help_dialog);
	xml->get_widget("toolbar", _toolbar);
	xml->get_widget("open_menuitem", _menu_file_open);
	xml->get_widget("save_menuitem", _menu_file_save);
	xml->get_widget("save_as_menuitem", _menu_file_save_as);
	xml->get_widget("quit_menuitem", _menu_file_quit);
	xml->get_widget("import_midi_menuitem", _menu_import_midi);
	xml->get_widget("export_midi_menuitem", _menu_export_midi);
	xml->get_widget("export_graphviz_menuitem", _menu_export_graphviz);
	xml->get_widget("view_toolbar_menuitem", _menu_view_toolbar);
	xml->get_widget("view_labels_menuitem", _menu_view_labels);
	//xml->get_widget("view_refresh_menuitem", _menu_view_refresh);
	//xml->get_widget("view_messages_menuitem", _menu_view_messages);
	xml->get_widget("help_about_menuitem", _menu_help_about);
	xml->get_widget("help_help_menuitem", _menu_help_help);
	xml->get_widget("canvas_scrolledwindow", _canvas_scrolledwindow);
	xml->get_widget("status_text", _status_text);
	xml->get_widget("main_paned", _main_paned);
	xml->get_widget("messages_expander", _messages_expander);
	xml->get_widget("slave_radiobutton", _slave_radiobutton);
	xml->get_widget("bpm_radiobutton", _bpm_radiobutton);
	xml->get_widget("bpm_spinbutton", _bpm_spinbutton);
	xml->get_widget("quantize_checkbutton", _quantize_checkbutton);
	xml->get_widget("quantize_spinbutton", _quantize_spinbutton);
	xml->get_widget("record_but", _record_button);
	xml->get_widget("stop_but", _stop_button);
	xml->get_widget("play_but", _play_button);
	xml->get_widget("zoom_normal_but", _zoom_normal_button);
	xml->get_widget("zoom_full_but", _zoom_full_button);
	xml->get_widget("arrange_but", _arrange_button);
	
	_canvas_scrolledwindow->add(*_canvas);
	_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(this,
				&MachinaGUI::scrolled_window_event));
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                       static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	//_zoom_slider->signal_value_changed().connect(sigc::mem_fun(this, &MachinaGUI::zoom_changed));
	
	_record_button->signal_toggled().connect(sigc::mem_fun(this, &MachinaGUI::record_toggled));
	_stop_button->signal_clicked().connect(sigc::mem_fun(this, &MachinaGUI::stop_clicked));
	_play_button->signal_toggled().connect(sigc::mem_fun(this, &MachinaGUI::play_toggled));

	_zoom_normal_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::zoom), 1.0));
	
	_zoom_full_button->signal_clicked().connect(sigc::mem_fun(_canvas.get(), &MachinaCanvas::zoom_full));
	_arrange_button->signal_clicked().connect(sigc::mem_fun(_canvas.get(), &MachinaCanvas::arrange));
	

	_menu_file_open->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_open));
	_menu_file_save->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save));
	_menu_file_save_as->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save_as));
	_menu_file_quit->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_quit));
	_menu_import_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_import_midi));
	_menu_export_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_midi));
	_menu_export_graphviz->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_graphviz));
	//_menu_view_refresh->signal_activate().connect(
	//	sigc::mem_fun(this, &MachinaGUI::menu_view_refresh));
	_menu_view_toolbar->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_toolbar_toggled));
	_menu_view_labels->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_labels_toggled));
	//_menu_view_messages->signal_toggled().connect(
	//	sigc::mem_fun(this, &MachinaGUI::show_messages_toggled));
	_menu_help_about->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_about));
	_menu_help_help->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_help));
	_slave_radiobutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::tempo_changed));
	_bpm_radiobutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::tempo_changed));
	_bpm_spinbutton->signal_changed().connect(
		sigc::mem_fun(this, &MachinaGUI::tempo_changed));
	_quantize_checkbutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_changed));
	_quantize_spinbutton->signal_changed().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_changed));

	connect_widgets();
		
	//update_state();

	_canvas->show();

	_main_window->present();

	_update_pane_position = false;
	_main_paned->set_position(max_pane_position());

	_main_paned->property_position().signal_changed().connect(
		sigc::mem_fun(*this, &MachinaGUI::on_pane_position_changed));
	
	_messages_expander->property_expanded().signal_changed().connect(
		sigc::mem_fun(*this, &MachinaGUI::on_messages_expander_changed));
	
	_main_paned->set_position(max_pane_position());
	_user_pane_position = max_pane_position() - _main_window->get_height()/8;
	_update_pane_position = true;
	_pane_closed = true;

	_bpm_radiobutton->set_active(true);
	_quantize_checkbutton->set_active(false);

	// Idle callback to drive the maid (collect garbage)
	Glib::signal_timeout().connect(
		sigc::bind_return(
			sigc::mem_fun(_maid.get(), &Raul::Maid::cleanup),
			true),
		1000);
	
	// Idle callback to update node states
	Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::idle_callback), 100);
	
	// Faster idle callback to update DSP load progress bar
	//Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::update_load), 50);
	
	_canvas->build(engine->machine());
}


MachinaGUI::~MachinaGUI() 
{
}


void
MachinaGUI::attach()
{
#if 0
	_jack_driver->attach(true);

	menu_view_refresh();

	update_toolbar();

	//m_status_bar->push("Connected to JACK server");
#endif
}
	

bool
MachinaGUI::idle_callback() 
{
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		const SharedPtr<NodeView> nv = PtrCast<NodeView>(*i);
		if (nv)
			nv->update_state();
	}

	return true;
}


bool
MachinaGUI::scrolled_window_event(GdkEvent* event)
{
	if (event->type == GDK_KEY_PRESS) {
		if (event->key.keyval == GDK_Delete) {
			
			ItemList selection = _canvas->selected_items();
			_canvas->clear_selection();

			for (ItemList::iterator i = selection.begin();
					i != selection.end(); ++i) {
				SharedPtr<NodeView> view = PtrCast<NodeView>(*i);
				if (view) {
					machine()->remove_node(view->node());
					_canvas->remove_item(view);
				}
			}

			return true;
		}
	}
	
	return false;
}


void
MachinaGUI::update_toolbar()
{
	_record_button->set_active(_engine->driver()->recording());
	_play_button->set_active(_engine->machine()->is_activated());
	_bpm_spinbutton->set_sensitive(_bpm_radiobutton->get_active());
	_quantize_spinbutton->set_sensitive(_quantize_checkbutton->get_active());
}


void
MachinaGUI::quantize_changed()
{
	if (_quantize_checkbutton->get_active()) {
		_engine->set_quantization(1/(double)_quantize_spinbutton->get_value_as_int());
	} else {
		_engine->set_quantization(0.0);
	}
	update_toolbar();
}


void
MachinaGUI::tempo_changed()
{
	_engine->set_bpm(_bpm_spinbutton->get_value_as_int());
}


void
MachinaGUI::zoom(double z)
{
	_canvas->set_zoom(z);
}


void
MachinaGUI::zoom_changed() 
{
/*
	static bool enable_signal = true;
	if (enable_signal) {
		enable_signal = false;
		zoom(_zoom_slider->get_value());
		enable_signal = true;
	}
	*/
}


#if 0
void
MachinaGUI::update_state()
{
	for (ModuleMap::iterator i = _canvas->modules().begin(); i != _canvas->modules().end(); ++i)
		(*i).second->load_location();

	//cerr << "[Machina] Resizing window: (" << _state_manager->get_window_size().x
	//	<< "," << _state_manager->get_window_size().y << ")" << endl;

	_main_window->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));
	
	//cerr << "[Machina] Moving window: (" << _state_manager->get_window_location().x
	//	<< "," << _state_manager->get_window_location().y << ")" << endl;
	
	_main_window->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));

}

#endif


void
MachinaGUI::status_message(const string& msg) 
{
	if (_status_text->get_buffer()->size() > 0)
		_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), "\n");

	_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), msg);
	_status_text->scroll_to_mark(_status_text->get_buffer()->get_insert(), 0);
}


/** Update the sensitivity status of menus to reflect the present.
 */
void
MachinaGUI::connect_widgets()
{
}

using namespace std;


void
MachinaGUI::menu_file_quit() 
{
	_main_window->hide();
}


void
MachinaGUI::menu_file_open() 
{
	Gtk::FileChooserDialog dialog(*_main_window, "Open Machine", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_local_only(false);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		//SharedPtr<Machina::Machine> new_machine = _engine->load_machine(dialog.get_uri());
		SharedPtr<Machina::Machine> new_machine = _engine->import_machine(dialog.get_uri());
		if (new_machine) {
			_canvas->destroy();
			_canvas->build(new_machine);
			_save_uri = dialog.get_uri();
		}
	}
}


void
MachinaGUI::menu_file_save() 
{
	if (_save_uri == "" || _save_uri.substr(0, 5) != "file:") {
		menu_file_save_as();
	} else {
		char* save_filename = raptor_uri_uri_string_to_filename(
				(const unsigned char*)_save_uri.c_str());
		if (!save_filename) {
			cerr << "ERROR: Unable to create filename from \"" << _save_uri << "\"" << endl;
			menu_file_save_as();
		}
		Raul::RDFWriter writer;
		cout << "Writing machine to " << save_filename << endl;
		writer.start_to_filename(save_filename);
		machine()->write_state(writer);
		writer.finish();
		free(save_filename);
	}
}


void
MachinaGUI::menu_file_save_as() 
{
	Gtk::FileChooserDialog dialog(*_main_window, "Save Machine",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);	
	
	if (_save_uri.length() > 0)
		dialog.set_uri(_save_uri);
	
	const int result = dialog.run();
	
	assert(result == Gtk::RESPONSE_OK
			|| result == Gtk::RESPONSE_CANCEL
			|| result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK) {	
		string filename = dialog.get_filename();

		if (filename.length() < 8 || filename.substr(filename.length()-8) != ".machina")
			filename += ".machina";
			
		bool confirm = false;
		std::fstream fin;
		fin.open(filename.c_str(), std::ios::in);
		if (fin.is_open()) {  // File exists
			string msg = "A file named \"";
			msg += filename + "\" already exists.\n\nDo you want to replace it?";
			Gtk::MessageDialog confirm_dialog(dialog,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confirm_dialog.run() == Gtk::RESPONSE_YES)
				confirm = true;
			else
				confirm = false;
		} else {  // File doesn't exist
			confirm = true;
		}
		fin.close();
		
		if (confirm) {
			Raul::RDFWriter writer;
			writer.start_to_filename(filename);
			_engine->machine()->write_state(writer);
			writer.finish();
			_save_uri = dialog.get_uri();
		}
	}
}


void
MachinaGUI::menu_import_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Learn from MIDI file",
			Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	
	Gtk::HBox* extra_widget = Gtk::manage(new Gtk::HBox());
	Gtk::SpinButton* length_sb = Gtk::manage(new Gtk::SpinButton());
	length_sb->set_increments(1, 10);
	length_sb->set_range(0, INT_MAX);
	length_sb->set_value(0);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("")), true, true);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("Maximum Length (0 = unlimited): ")), false, false);
	extra_widget->pack_start(*length_sb, false, false); 
	dialog.set_extra_widget(*extra_widget);
	extra_widget->show_all();
	

	/*Gtk::HBox* extra_widget = Gtk::manage(new Gtk::HBox());
	Gtk::SpinButton* track_sb = Gtk::manage(new Gtk::SpinButton());
	track_sb->set_increments(1, 10);
	track_sb->set_range(1, 256);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("")), true, true);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("Track: ")), false, false);
	extra_widget->pack_start(*track_sb, false, false); 
	dialog.set_extra_widget(*extra_widget);
	extra_widget->show_all();
	*/

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());
		//SharedPtr<Machina::Machine> machine = file_driver->learn(dialog.get_uri(),
		//		track_sb->get_value_as_int());
		
		double length = length_sb->get_value_as_int();

		SharedPtr<Machina::Machine> machine = file_driver->learn(dialog.get_filename(), 0.0, length);
		
		if (machine) {
			dialog.hide();
			machine->activate();
			machine->reset();
			_canvas->build(machine);
			_engine->driver()->set_machine(machine);
		} else {
			Gtk::MessageDialog msg_dialog(dialog, "Error loading MIDI file",
					false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			msg_dialog.run();
		} 
	}
}


void
MachinaGUI::menu_export_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a MIDI file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());
		_engine->driver()->deactivate();
		const SharedPtr<Machina::Machine> m = _engine->machine();
		m->set_sink(file_driver->writer());
		file_driver->writer()->start(dialog.get_filename());
		file_driver->run(m, 32); // FIXME: hardcoded max length.  TODO: solve halting problem
		m->set_sink(_engine->driver());
		m->reset();
		file_driver->writer()->finish();
		_engine->driver()->activate();
	}
}


void
MachinaGUI::menu_export_graphviz()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a GraphViz DOT file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK)
		_canvas->render_to_dot(dialog.get_filename());
}


void
MachinaGUI::on_pane_position_changed()
{
	// avoid infinite recursion...
	if (!_update_pane_position)
		return;

	_update_pane_position = false;

	int new_position = _main_paned->get_position();

	if (_pane_closed && new_position < max_pane_position()) {
		// Auto open
		_user_pane_position = new_position;
		_messages_expander->set_expanded(true);
		_pane_closed = false;
		//_menu_view_messages->set_active(true);
	} else if (new_position >= max_pane_position()) {
		// Auto close
		_pane_closed = true;

		_messages_expander->set_expanded(false);
		if (new_position > max_pane_position())
			_main_paned->set_position(max_pane_position()); // ... here
		//_menu_view_messages->set_active(false);
		
		_user_pane_position = max_pane_position() - _main_window->get_height()/8;
	}

	_update_pane_position = true;
}


void
MachinaGUI::on_messages_expander_changed()
{
	if (!_pane_closed) {
		// Store pane position for restoring
		_user_pane_position = _main_paned->get_position();
		if (_update_pane_position) {
			_update_pane_position = false;
			_main_paned->set_position(max_pane_position());
			_update_pane_position = true;
		}
		_pane_closed = true;
	} else {
		_main_paned->set_position(_user_pane_position);
		_pane_closed = false;
	}
}

/*
void
MachinaGUI::show_messages_toggled()
{
	if (_update_pane_position)
		//_messages_expander->set_expanded(_menu_view_messages->get_active());
}
*/

void
MachinaGUI::show_toolbar_toggled()
{
	if (_menu_view_toolbar->get_active())
		_toolbar->show();
	else
		_toolbar->hide();
}


void
MachinaGUI::show_labels_toggled()
{
	const bool show = _menu_view_labels->get_active();
	
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		const SharedPtr<NodeView> nv = PtrCast<NodeView>(*i);
		if (nv)
			nv->show_label(show);
	}

	for (ConnectionList::iterator c = _canvas->connections().begin();
			c != _canvas->connections().end(); ++c) {
		const SharedPtr<EdgeView> ev = PtrCast<EdgeView>(*c);
		if (ev)
			ev->show_label(show);
	}
}


/*
void
MachinaGUI::menu_view_refresh() 
{
	assert(_canvas);
	
	//_canvas->destroy();
}
*/

void
MachinaGUI::menu_help_about() 
{
	_about_window->set_transient_for(*_main_window);
	_about_window->show();
}


void
MachinaGUI::menu_help_help() 
{
	_help_dialog->set_transient_for(*_main_window);
	_help_dialog->run();
	_help_dialog->hide();
}


void
MachinaGUI::record_toggled()
{
	if (_record_button->get_active() && ! _engine->driver()->recording()) {
		_engine->driver()->start_record();
	} else if (_engine->driver()->recording()) {
		_engine->driver()->finish_record();
		_canvas->build(_engine->machine());
		update_toolbar();
	}
}


void
MachinaGUI::stop_clicked()
{
	if (_engine->driver()->recording()) {
		_engine->driver()->finish_record();
	} else {
		_engine->machine()->deactivate();
		_engine->driver()->reset();
	}

	update_toolbar();
}


void
MachinaGUI::play_toggled()
{
	if (_play_button->get_active())
		_engine->machine()->activate();
	else
		_engine->machine()->deactivate();
}


