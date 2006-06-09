/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <iostream>
#include <libgnomecanvasmm.h>

#include "Patchage.h"
#include "JackDriver.h"

#ifdef HAVE_LASH
#include <lash/lash.h>
#endif // HAVE_LASH

int main(int argc, char** argv)
{
	try {
		
	Gnome::Canvas::init();
	Gtk::Main app(argc, argv);
	
	Patchage patchage(argc, argv);
	patchage.attach();

	app.run(*patchage.window());
	
	} catch (std::string msg) {
		std::cerr << "Caught exception, aborting.  Error message was: " << msg << std::endl;
		return 1;
	}

	return 0;
}


