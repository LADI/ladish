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

#ifndef LASHDRIVER_H
#define LASHDRIVER_H

#include <lash/lash.h>
#include <raul/LashServerInterface.hpp>
#include "Driver.hpp"

class Patchage;

class LashDriver : public Driver {
public:
	LashDriver(Patchage* app, int argc, char** argv);
	~LashDriver();
	
	void attach(bool launch_daemon);
	void detach();

	bool is_attached() const
	{ return _server_interface && _server_interface->enabled(); }
	
	boost::shared_ptr<PatchagePort> find_port_view(
			Patchage*                     patchage,
			const PatchageEvent::PortRef& ref) {
		return boost::shared_ptr<PatchagePort>();
	}
	
	boost::shared_ptr<PatchagePort> create_port_view(
			Patchage*                     patchage,
			const PatchageEvent::PortRef& ref) {
		return boost::shared_ptr<PatchagePort>();
	}
	
	bool connect(boost::shared_ptr<PatchagePort>, boost::shared_ptr<PatchagePort>)
	{ return false; }

	bool disconnect(boost::shared_ptr<PatchagePort>, boost::shared_ptr<PatchagePort>)
	{ return false; }

	void refresh() {}

	void process_events(Patchage* app) { _server_interface->process_events(); }

	void restore_project(const std::string& directory);
	void set_project_directory(const std::string& directory);
	void save_project();
	void close_project();

private:
	Patchage*   _app;
	std::string _project_name;

	lash_args_t*                         _args;
	SharedPtr<Raul::LashServerInterface> _server_interface;

	void on_project_add(const SharedPtr<Raul::LashProject> project);
	void on_save_file(const std::string& directory);
	void on_restore_file(const std::string& directory);
	void on_quit();

	void handle_event(lash_event_t* conf);
	void handle_config(lash_config_t* conf);
};


#endif // LASHDRIVER_H
