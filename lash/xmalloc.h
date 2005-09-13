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
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_XMALLOC_H__
#define __LASH_XMALLOC_H__


#ifdef LASH_BUILD
#define _GNU_SOURCE
#include "config.h"
#endif /* LASH_BUILD */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LASH_DEBUG

#define lash_malloc   malloc
#define lash_realloc  realloc
#define lash_strdup   strdup

#else

#define lash_malloc   lash_xmalloc
#define lash_realloc  lash_xrealloc
#define lash_strdup   lash_xstrdup

#endif

void * lash_malloc0  (size_t);


void * lash_xmalloc  (size_t);
void * lash_xrealloc (void *, size_t);
char * lash_xstrdup  (const char *);



#ifdef __cplusplus
}
#endif

#endif /* __LASH_XMALLOC_H__ */
