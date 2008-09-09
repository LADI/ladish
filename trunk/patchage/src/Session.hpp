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

#ifndef PATCHAGE_SESSION_HPP
#define PATCHAGE_SESSION_HPP

#include <boost/shared_ptr.hpp>

struct SessionImpl;
class Project;
class LashClient;

class Session
{
public:
	Session();
	~Session();

	void clear();

	void project_add(boost::shared_ptr<Project> project);
	void project_close(const std::string& project_name);

	boost::shared_ptr<Project> find_project_by_name(const std::string& name);

	void client_add(boost::shared_ptr<LashClient> client);
	void client_remove(const std::string& id);

	boost::shared_ptr<LashClient> find_client_by_id(const std::string& id);

	sigc::signal< void, boost::shared_ptr<Project> > _signal_project_added;
	sigc::signal< void, boost::shared_ptr<Project> > _signal_project_closed;

private:
	SessionImpl* _impl;
};

#endif // PATCHAGE_SESSION_HPP

