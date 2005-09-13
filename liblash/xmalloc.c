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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lash/lash.h>

#ifndef LASH_DEBUG

void *
lash_xmalloc (size_t size)
{
  void * ptr;
  
  ptr = malloc (size);
  
  if (!ptr)
    {
      fprintf (stderr, "%s: could not allocate memory; aborting\n",
               __FUNCTION__);
      abort ();
    }
  
  return ptr;
}

void *
lash_xrealloc (void * data, size_t size)
{
  void * ptr;
  
  ptr = realloc (data, size);
  
  if (!ptr)
    {
      fprintf (stderr, "%s: could not allocate memory; aborting\n",
               __FUNCTION__);
      abort ();
    }
  
  return ptr;
}

char *
lash_xstrdup (const char * string)
{
  void * str;
  
  str = strdup (string);
  
  if (!str)
    {
      fprintf (stderr, "%s: could not allocate memory; aborting\n",
               __FUNCTION__);
      abort ();
    }
  
  return str;
}

#endif /* LASH_DEBUG */

void *
lash_malloc0  (size_t size)
{
  void * data;

  data = lash_malloc (size);

  memset (data, 0, size);

  return data;
}
