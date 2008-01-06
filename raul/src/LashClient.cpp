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

#include <raul/LashClient.hpp>

using namespace std;

namespace Raul {


LashClient::LashClient(lash_client_t* lash_client)
	: _lash_client(lash_client)
{
}


LashClient::~LashClient()
{
}



SharedPtr<LashClient>
LashClient::create(
		lash_args_t*  args,
        const string& name,
        int           flags)
{
	SharedPtr<LashClient> result;

	lash_client_t* client = lash_init(args, name.c_str(),
			flags, LASH_PROTOCOL(2, 0));

	if (client)
		result = SharedPtr<LashClient>(new LashClient(client));

	return result;
}

	
/** Process all pending LASH events.
 *
 * This should be called regularly to keep track of LASH state.
 */
void
LashClient::process_events()
{
	lash_event_t*  ev = NULL;
	lash_config_t* conf = NULL;

	// Process events
	while ((ev = lash_get_event(_lash_client)) != NULL) {
		handle_event(ev);
		lash_event_destroy(ev);	
	}

	// Process configs
	while ((conf = lash_get_config(_lash_client)) != NULL) {
		handle_config(conf);
		lash_config_destroy(conf);	
	}
}


} // namespace Raul
