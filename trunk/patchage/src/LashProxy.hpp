// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
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

#ifndef PATCHAGE_LASH_PROXY_HPP
#define PATCHAGE_LASH_PROXY_HPP

#include "Patchage.hpp"

struct ProjectInfo {
	std::string name;
	time_t      modification_time;
	std::string description;
};

struct LoadedProjectProperties {
	bool        modified_status;
	std::string description;
	std::string notes;
};

class Patchage;
class Session;
class LashProxyImpl;

class LashProxy {
public:
	LashProxy(Patchage* app, Session* session);
	~LashProxy();

	void get_available_projects(std::list<ProjectInfo>& projects);
	void load_project(const std::string& project_name);
	void save_all_projects();
	void save_project(const std::string& project_name);
	void close_project(const std::string& project_name);
	void close_all_projects();
	void project_rename(const std::string& old_name, const std::string& new_name);
	void get_loaded_project_properties(const std::string& name, LoadedProjectProperties& properties);
	void project_set_description(const std::string& project_name, const std::string& description);
	void project_set_notes(const std::string& project_name, const std::string& notes);

private:
	LashProxyImpl* _impl;
};

#endif // PATCHAGE_LASH_PROXY_HPP
