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

#ifndef PATCHAGE_LASH_CLIENT_HPP
#define PATCHAGE_LASH_CLIENT_HPP

#include <string>
#include <sigc++/signal.h>

class LashClientImpl;
class LashProxy;
class LashProxyImpl;
class Project;

class LashClient
{
public:
	LashClient(
		LashProxy* proxy,
		Project* project,
		const std::string& id,
		const std::string& name);

	~LashClient();

	Project*
	get_project();

	const std::string& get_id() const;
	const std::string& get_name() const;

	void do_rename(const std::string& name);

	sigc::signal<void> _signal_renamed;

private:
	friend class LashProxyImpl;

	void
	on_name_changed(const std::string& name);

	LashClientImpl* _impl;
};

#endif // PATCHAGE_LASH_CLIENT_HPP
