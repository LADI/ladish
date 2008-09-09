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

#ifndef LASH_PROJECT_HPP
#define LASH_PROJECT_HPP

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>
#include <sigc++/signal.h>

struct ProjectImpl;
class LashProxy;
class LashProxyImpl;
class LashClient;

class Project {
public:
	Project(LashProxy* proxy, const std::string& name);

	~Project();

	void clear();
	
	typedef std::list< boost::shared_ptr<LashClient> > Clients;

	const std::string& get_name() const;
	const std::string& get_description() const;
	const std::string& get_notes() const;
	const Clients&     get_clients() const;
	bool               get_modified_status() const;

	void do_rename(const std::string& name);
	void do_change_description(const std::string& description);
	void do_change_notes(const std::string& notes);

	sigc::signal<void> _signal_renamed;
	sigc::signal<void> _signal_modified_status_changed;
	sigc::signal<void> _signal_description_changed;
	sigc::signal<void> _signal_notes_changed;

	sigc::signal< void, boost::shared_ptr<LashClient> > _signal_client_added;
	sigc::signal< void, boost::shared_ptr<LashClient> > _signal_client_removed;

private:
	friend class LashProxyImpl;

	void on_name_changed(const std::string& name);
	void on_modified_status_changed(bool modified_status);
	void on_description_changed(const std::string& description);
	void on_notes_changed(const std::string& notes);
	void on_client_added(boost::shared_ptr<LashClient> client);
	void on_client_removed(const std::string& id);

	ProjectImpl* _impl;
};

#endif // LASH_PROJECT_HPP
