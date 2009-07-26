/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __LASHD_FILE_H__
#define __LASHD_FILE_H__

#include <stdlib.h>
#include <stdbool.h>

#include "../../common/safety.h"

#define get_store_and_return_fqn(dir, file)   \
  static char *fqn = NULL;                    \
  if (fqn)                                    \
    free(fqn);                                \
  fqn = lash_strdup(lash_get_fqn(dir, file)); \
  return fqn;

void
lash_create_dir(const char *dir);

/* WARNING: Deletes all files in the directory,
   and recurses into sub-directories */
void
lash_remove_dir(const char *dir);

/* The returned string is safe for use in nested invocations */
/* deprecated, use lash_dup_fqn() in new code */
const char *
lash_get_fqn(const char *dir,
             const char *file);

/* free with lash_free() */
char *
lash_dup_fqn(const char *dir,
             const char *append);

bool
lash_file_exists(const char *file);

bool
lash_dir_exists(const char *dir);

bool
lash_dir_empty(const char *dir);

bool
lash_read_text_file(const char  *file_path,
                    char       **ptr);

#endif /* __LASHD_FILE_H__ */
