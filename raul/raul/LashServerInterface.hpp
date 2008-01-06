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

#ifndef RAUL_LASH_SERVER_INTERFACE_HPP
#define RAUL_LASH_SERVER_INTERFACE_HPP

#include <string>
#include <list>
#include <lash/lash.h>
#include <raul/LashClient.hpp>
#include <raul/LashProject.hpp>
#include <raul/SharedPtr.hpp>
#include <sigc++/sigc++.h>

namespace Raul {


class LashServerInterface : public LashClient, public sigc::trackable {
public:
	static SharedPtr<LashServerInterface> create(
			lash_args_t*         args,
	        const std::string&   name,
	        int                  flags);

	sigc::signal<void>                                signal_quit;
	sigc::signal<void, const SharedPtr<LashProject> > signal_project_add;
	sigc::signal<void, const SharedPtr<LashProject> > signal_project_remove;

	void restore_project(const std::string& filename);

	typedef std::list<SharedPtr<LashProject> > Projects;
	Projects& projects() { return _projects; }

	SharedPtr<LashProject> project(const std::string& name);

private:
	LashServerInterface(lash_client_t* client);
	void handle_event(lash_event_t* ev);
	void project_closed(const std::string& name);

	Projects _projects;
};


} // namespace Raul

#endif // RAUL_LASH_SERVER_INTERFACE_HPP

