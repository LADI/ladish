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

#ifndef RAUL_LASH_CLIENT_H
#define RAUL_LASH_CLIENT_H

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <lash/lash.h>
#include <raul/SharedPtr.h>

namespace Raul {


class LashClient : public boost::enable_shared_from_this<LashClient> {
public:
	virtual ~LashClient();

	static SharedPtr<LashClient> create(
			lash_args_t*       args,
	        const std::string& name,
	        int                flags);
	
	lash_client_t* lash_client() { return _lash_client; };

	bool enabled() { return lash_enabled(_lash_client); }

	void process_events();

protected:
	LashClient(lash_client_t* client);
	
	virtual void handle_event(lash_event_t*)   {}
	virtual void handle_config(lash_config_t*) {}
	
	friend class LashProject;
	virtual void project_closed(const std::string& /*name*/) {}
	
	lash_client_t* _lash_client;
};


} // namespace Raul

#endif // RAUL_LASH_CLIENT_H

