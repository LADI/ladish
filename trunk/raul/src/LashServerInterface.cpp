/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include "raul/LashServerInterface.hpp"

using namespace std;

namespace Raul {


LashServerInterface::LashServerInterface(lash_client_t* lash_client)
	: LashClient(lash_client)
{
}


SharedPtr<LashServerInterface>
LashServerInterface::create(
		lash_args_t*    args,
        const string&   name,
		int             flags)
{
	SharedPtr<LashServerInterface> result;

	lash_client_t* client = lash_init(args, name.c_str(),
			flags | LASH_Server_Interface, LASH_PROTOCOL(2, 0));

	if (client)
		result = SharedPtr<LashServerInterface>(new LashServerInterface(client));

	return result;
}


void
LashServerInterface::handle_event(lash_event_t* ev)
{
	LASH_Event_Type type = lash_event_get_type(ev);
	const char*     c_str = lash_event_get_string(ev);
	const string    str   = (c_str == NULL) ? "" : c_str;

	SharedPtr<LashProject> p;
	
	//cout << "[LashServerInterface] Event, type = " << (unsigned int)type
	//	<< ", string = " << str << endl;
	
	switch (type) {
	case LASH_Project_Add:
		p = project(str);
		if (!p) {
			SharedPtr<LashProject> project(new LashProject(shared_from_this(), str));
			_projects.push_back(project);
			signal_project_add.emit(project);
		}
		break;
	case LASH_Project_Remove:
		p = project(lash_event_get_project(ev));
		if (p) {
			signal_project_remove.emit(p);
			_projects.remove(p);
		}
		break;
	case LASH_Project_Dir:
		p = project(lash_event_get_project(ev));
		if (p) {
			p->set_directory(str);
			p->signal_directory.emit(str);
		}
		break;
	case LASH_Project_Name:
		p = project(lash_event_get_project(ev));
		if (p) {
			p->set_name(str);
			p->signal_name.emit(str);
		}
		break;
	case LASH_Client_Add:
		break;
	case LASH_Client_Name:
		break;
	case LASH_Jack_Client_Name:
		break;
	case LASH_Alsa_Client_ID:
		break;
	case LASH_Percentage:
		break;
	case LASH_Save:
		break;
	case LASH_Restore_Data_Set:
		break;
	case LASH_Server_Lost:
		break;
	case LASH_Client_Remove:
		break;
	case LASH_Save_File:
		break;
	case LASH_Restore_File:
		break;
	case LASH_Save_Data_Set:
		break;
	case LASH_Quit:
		_lash_client = NULL;
		signal_quit.emit();
		break;
	}

	LashClient::handle_event(ev);
}
	

void
LashServerInterface::restore_project(const std::string& filename)
{
	cout << "[LASH] Restoring project at " << filename << endl;
	lash_event_t* event = lash_event_new_with_type(LASH_Project_Add);
	lash_event_set_string(event, filename.c_str());
	lash_send_event(lash_client(), event);
}


bool
project_comparator(SharedPtr<LashProject> project, const std::string& name)
{
	return (project->name() == name);
}


SharedPtr<LashProject>
LashServerInterface::project(const std::string& name)
{
	for (Projects::iterator p = _projects.begin(); p != _projects.end(); ++p)
		if ((*p)->name() == name)
			return *p;

	return SharedPtr<LashProject>();
}


void
LashServerInterface::project_closed(const string& name)
{
	for (Projects::iterator p = _projects.begin(); p != _projects.end(); ++p) {
		if ((*p)->name() == name) {
			_projects.erase(p);
			break;
		}
	}
}


} // namespace Raul
