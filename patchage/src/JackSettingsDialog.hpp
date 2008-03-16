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

#ifndef PATCHAGE_JACK_SETTINGS_DIALOG_HPP
#define PATCHAGE_JACK_SETTINGS_DIALOG_HPP

#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm/dialog.h>
#include <fstream>
#include <string>


class JackSettingsDialog : public Gtk::Dialog {
public:
	JackSettingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
		: Gtk::Dialog(cobject)
	{
		xml->get_widget("jack_settings_command_entry", _command_entry);
		xml->get_widget("jack_settings_cancel_but", _cancel_but);
		xml->get_widget("jack_settings_ok_but", _ok_but);

		_cancel_but->signal_clicked().connect(
				sigc::mem_fun(this, &JackSettingsDialog::on_cancel));
		_ok_but->signal_clicked().connect(
				sigc::mem_fun(this, &JackSettingsDialog::on_ok));
		
		_command_entry->set_text(current_jack_command());
	}

private:
	void on_cancel() { hide(); }

	std::string current_jack_command() {
		std::string result;

		const char* const home = getenv("HOME");
		if (home) {
			std::string jackdrc_path(home);
			jackdrc_path += "/.jackdrc";

			std::ifstream jackdrc(jackdrc_path.c_str());
			std::getline(jackdrc, result);
			jackdrc.close();
		}

		return result;
	}

	void on_ok() {
		hide();
		const char* const home = getenv("HOME");
		if (home) {
			std::string jackdrc_path(home);
			jackdrc_path += "/.jackdrc";
			
			std::ofstream jackdrc(jackdrc_path.c_str());
			jackdrc << _command_entry->get_text() << std::endl;
			jackdrc.close();
		}
	}

	Gtk::Entry*  _command_entry;
	Gtk::Button* _cancel_but;
	Gtk::Button* _ok_but;
};


#endif // PATCHAGE_JACK_SETTINGS_DIALOG_HPP

