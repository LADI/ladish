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

#ifndef JACK_SETTINGS_DIALOG_H
#define JACK_SETTINGS_DIALOG_H

#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm/dialog.h>
#include <fstream>


class JackSettingsDialog : public Gtk::Dialog {
public:
	JackSettingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
	{
		xml->get_widget("jack_settings_command_entry", _command_entry);
		xml->get_widget("jack_settings_cancel_button", _cancel_button);
		xml->get_widget("jack_settings_ok_button", _ok_button);

		_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &JackSettingsDialog::on_cancel));
		_ok_button->signal_clicked().connect(sigc::mem_fun(this, &JackSettingsDialog::on_ok));
		_command_entry->set_text(current_jack_command());
	}

private:
	void on_cancel()
	{
		hide();
	}

	string current_jack_command()
	{
		string result;

		const char* const home = getenv("HOME");
		if (home) {
			string jackdrc_path(home);
			jackdrc_path += "/.jackdrc";

			ifstream jackdrc(jackdrc_path.c_str());
			std::getline(jackdrc, result);
			jackdrc.close();
		}

		return result;
	}

	void on_ok()
	{
		hide();
		const char* const home = getenv("HOME");
		if (home) {
			string jackdrc_path(home);
			jackdrc_path += "/.jackdrc";
			
			ofstream jackdrc(jackdrc_path.c_str());
			jackdrc << _command_entry->get_text() << endl;
			jackdrc.close();
		}
	}


	Gtk::Entry*  _command_entry;
	Gtk::Button* _cancel_button;
	Gtk::Button* _ok_button;
};


#endif // JACK_SETTINGS_DIALOG_H

