// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "Project.hpp"
#include "Session.hpp"
#include "LashClient.hpp"

using namespace std;
using boost::shared_ptr;

struct SessionImpl {
	list< shared_ptr<Project> >    projects;
	list< shared_ptr<LashClient> > clients;
};

Session::Session()
{
	_impl = new SessionImpl;
}

Session::~Session()
{
	delete _impl;
}

void
Session::clear()
{
	shared_ptr<Project> project;

	_impl->clients.clear();

	while (!_impl->projects.empty()) {
		project = _impl->projects.front();
		_impl->projects.pop_front();
		project->clear();
		_signal_project_closed.emit(project);
	}
}

void
Session::project_add(shared_ptr<Project> project)
{
	_impl->projects.push_back(project);

	_signal_project_added.emit(project);
}

shared_ptr<Project>
Session::find_project_by_name(const string& name)
{
	shared_ptr<Project> project;
	for (list< shared_ptr<Project> >::iterator i = _impl->projects.begin(); i != _impl->projects.end(); i++)
		if ((*i)->get_name() == name)
			return (*i);

	return shared_ptr<Project>();
}

void
Session::project_close(const string& project_name)
{
	shared_ptr<Project> project;
	Project::Clients clients;

	for (list<shared_ptr<Project> >::iterator iter = _impl->projects.begin(); iter != _impl->projects.end(); iter++) {
		project = *iter;

		if (project->get_name() == project_name) {
			_impl->projects.erase(iter);
			_signal_project_closed.emit(project);

			// remove clients from session, if not removed already
			clients = project->get_clients();
			for (Project::Clients::const_iterator i = clients.begin(); i != clients.end(); i++)
				client_remove((*i)->get_id());

			return;
		}
	}
}

void
Session::client_add(shared_ptr<LashClient> client)
{
	_impl->clients.push_back(client);
}

void
Session::client_remove(const string& id)
{
	for (list<shared_ptr<LashClient> >::iterator i = _impl->clients.begin(); i != _impl->clients.end(); i++) {
		if ((*i)->get_id() == id) {
			_impl->clients.erase(i);
			return;
		}
	}
}

shared_ptr<LashClient>
Session::find_client_by_id(const string& id)
{
	for (list<shared_ptr<LashClient> >::iterator i = _impl->clients.begin(); i != _impl->clients.end(); i++)
		if ((*i)->get_id() == id)
			return *i;

	return shared_ptr<LashClient>();
}

