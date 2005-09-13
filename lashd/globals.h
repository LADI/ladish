/*
 *   LASH
 *    
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASHD_GLOBALS_H__
#define __LASHD_GLOBALS_H__

#include <lash/lash.h>

#define get_store_and_return_fqn(dir, file) \
  static char * fqn = NULL; \
  \
  if (fqn) \
    free (fqn); \
  \
  fqn = lash_strdup (lash_get_fqn (dir, file)); \
  \
  return fqn;

#endif /* __LASHD_GLOBALS_H__ */
