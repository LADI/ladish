/* Find the location of the program in the filesytem.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * 
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/** Return the absolute path of the binary.
 * Returned value must be freed by caller.
 */
static char*
binary_location()
{
  Dl_info dli;
  dladdr((void*)&binary_location, &dli);

  char* bin_loc = (char*)calloc(PATH_MAX, sizeof(char));
  realpath(dli.dli_fname, bin_loc);

  return bin_loc;
}

