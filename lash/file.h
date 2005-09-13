/*
 *   LASH
 *    
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
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

#ifndef __LASH_FILE_H__
#define __LASH_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif


void         lash_create_dir  (const char * dir);

/* WARNING: deletes all files in the directory, and recurses into sub-directories
 */
void         lash_remove_dir  (const char * dir);

/* the returned string is safe until after the next invocation
 * (ie, it is safe to do lash_get_fqn (lash_get_fqn (fqdir, dir), file); )
 */
const char * lash_get_fqn (const char * fqdir, const char * file);

int          lash_dir_exists  (const char * fqdir);
int          lash_dir_empty   (const char * fqdir);
int          lash_file_exists (const char * file);


#ifdef __cplusplus
}
#endif

#endif /* __LASH_FILE_H__ */
