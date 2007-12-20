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

#include <iostream>
#include <string>
#include CONFIG_H_PATH
#include "LashDriver.hpp"
#include "Patchage.hpp"
#include "StateManager.hpp"

using namespace std;


LashDriver::LashDriver(Patchage* app, int argc, char** argv)
	: Driver(2)
	, _app(app)
	, _args(NULL)
{
	_args = lash_extract_args(&argc, &argv);
}


LashDriver::~LashDriver()
{
	if (_args)
		lash_args_destroy(_args);
}


void
LashDriver::attach(bool launch_daemon)
{
	// Already connected
	if (_server_interface && _server_interface->enabled())
		return;

	int lash_flags = LASH_Server_Interface | LASH_Config_File;
	if (!launch_daemon)
		lash_flags |= LASH_No_Start_Server;
	
	_server_interface = Raul::LashServerInterface::create(
			_args, PACKAGE_NAME, lash_flags);

	if (_server_interface) {
		_server_interface->signal_project_add.connect(sigc::mem_fun(this, &LashDriver::on_project_add));
		_server_interface->signal_quit.connect(sigc::mem_fun(this, &LashDriver::on_quit));
		signal_attached.emit();
		_app->status_message("[LASH] Attached");
	} else {
		_app->status_message("[LASH] Unable to attach to server");
	}
}


void
LashDriver::detach()
{
	_server_interface.reset();
	_app->status_message("[LASH] Detached");
	signal_detached.emit();
}


void
LashDriver::on_project_add(const SharedPtr<Raul::LashProject> project)
{
	_project_name = project->name();
	project->signal_save_file.connect(sigc::mem_fun(this, &LashDriver::on_save_file));
	project->signal_restore_file.connect(sigc::mem_fun(this, &LashDriver::on_restore_file));
}


void
LashDriver::on_save_file(const string& directory)
{
	cout << "[LashDriver] LASH Save File - " << directory << endl;
	_app->state_manager()->save(directory + "/locations");
}


void
LashDriver::on_restore_file(const string& directory)
{
	cout << "[LashDriver] LASH Restore File - " << directory << endl;
	_app->state_manager()->load(directory + "/locations");
	_app->update_state();
}


void
LashDriver::on_quit()
{
	cout << "[LashDriver] Quit" << endl;
}


void
LashDriver::handle_config(lash_config_t* conf)
{
	const char*    key      = NULL;
	const void*    val      = NULL;
	size_t         val_size = 0;

	cout << "[LashDriver] LASH Config.  Key = " << key << endl;

	key      = lash_config_get_key(conf);
	val      = lash_config_get_value(conf);
	val_size = lash_config_get_value_size(conf);
}
	

void
LashDriver::restore_project(const std::string& directory)
{
	_project_name = "";
	_server_interface->restore_project(directory);
}


void
LashDriver::set_project_directory(const std::string& directory)
{
	SharedPtr<Raul::LashProject> project = _server_interface->project(_project_name);
	
	if (project)
		project->set_directory(directory);
	else
		cerr << "[LashDriver] No LASH project to set directory!" << endl;
}


void
LashDriver::save_project()
{
	SharedPtr<Raul::LashProject> project = _server_interface->project(_project_name);
	
	if (project)
		project->save();
	else
		cerr << "[LashDriver] No LASH project to save!" << endl;
}

	
void
LashDriver::close_project()
{
	cerr << "CLOSE PROJECT\n";
	SharedPtr<Raul::LashProject> project = _server_interface->project(_project_name);
	
	if (project)
		project->close();
	else
		cerr << "[LashDriver] No LASH project to close!" << endl;
}



