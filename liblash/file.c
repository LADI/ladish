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

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
                     
#include <lash/lash.h>

int
lash_file_exists (const char * file)
{
  int err;
  struct stat filestat;
  
  err = stat (file, &filestat);
  if (err == -1)
    return 0;

  return 1;
}

int
lash_dir_exists (const char * dir)
{
  DIR * dirstream;
  
  dirstream = opendir (dir);
  if (dirstream)
    {
      closedir (dirstream);
      return 1;
    }
  
  return 0;  
}

const char *
lash_get_fqn (const char * param_dir, const char * param_file)
{
  static char * fqn = NULL;
  static size_t fqn_size = 32;
  size_t str_size;
  char * dir;
  char * file;
  
  dir  = lash_strdup (param_dir);
  file = lash_strdup (param_file);
  
  if (!fqn)
    fqn = lash_malloc (fqn_size);
  
  str_size = strlen (dir) + 1 + strlen (file) + 1;
  
  if (str_size > fqn_size)
    {
      fqn_size = str_size;
      fqn = lash_realloc (fqn, fqn_size);
    }
  
  sprintf (fqn, "%s/%s", dir, file);
  
  free (dir);
  free (file);
  
  return fqn;
}

void
lash_create_dir (const char * dir)
{
  char * parent, * ptr;
  DIR * dirstream;
  struct stat parentstat;
  int err;
  
  dirstream = opendir (dir);
  if (dirstream)
    {
      closedir (dirstream);
      return;
    }
  
  if (errno == EACCES)
    {
      fprintf (stderr, "%s: warning: directory '%s' already exists, but we don't have permission to read it\n",
               __FUNCTION__, dir);
      return;
    }
  
  /* try and create the parent */
  parent = lash_strdup (dir);
  ptr = strrchr (parent, '/');
  if (ptr)
    *ptr = '\0';
  
  if (strlen (parent))
    lash_create_dir (parent);
  
  if (!strlen (parent))
    {
      ptr[0] = '/';
      ptr[1] = '\0';
    }  
  
  /* create our dir */
  /* stat parent */
  err = stat (parent, &parentstat);
  free (parent);
  if (err == -1)
    {
      fprintf (stderr, "%s: could not stat parent '%s' in order to create dir '%s': %s\n",
               __FUNCTION__, parent, dir, strerror (errno));
      return;
    }
  
  err = mkdir (dir, parentstat.st_mode);
  if (err == -1)
    fprintf (stderr, "%s: could not create directory '%s': %s\n", __FUNCTION__, dir, strerror (errno));
}

int
lash_dir_empty (const char * dir)
{
  DIR * dirstream;
  struct dirent * entry;
  int empty = 1;
  
  dirstream = opendir (dir);
  if (!dirstream)
    {
      fprintf (stderr, "%s: could not open directory '%s' to check emptiness; returning false for saftey: %s\n",
               __FUNCTION__, dir, strerror (errno));
      return 0;
    }
  
  while ( (entry = readdir (dirstream)) )
    {
      if (strcmp (entry->d_name, ".") == 0 ||
          strcmp (entry->d_name, "..") == 0)
        continue;
      
      empty = 0;
      break;
    }
  
  closedir (dirstream);
  return empty;
}

void
lash_remove_dir  (const char * dirarg)
{
  DIR * dirstream;
  struct dirent * entry;
  int err;
  const char * fqn;
  struct stat stat_info;
  char * dir;

  dir = lash_strdup (dirarg);
    
  dirstream = opendir (dir);
  if (!dirstream)
    {
      fprintf (stderr, "%s: could not open directory '%s' to remove it: %s\n",
               __FUNCTION__, dir, strerror (errno));
      free (dir);
      return;
    }
  
  while ( (entry = readdir (dirstream)) )
    {
      if (strcmp (entry->d_name, ".") == 0 ||
          strcmp (entry->d_name, "..") == 0)
        continue;
      
      fqn = lash_get_fqn (dir, entry->d_name);

      err = stat (fqn, &stat_info);
      if (err)
	{
	  fprintf (stderr, "%s: could not stat file '%s': %s\n",
		   __FUNCTION__, fqn, strerror (errno));
	}
      else
	{
	  if (S_ISDIR(stat_info.st_mode))
	    {
	      lash_remove_dir (fqn);
	      continue;
	    }
	}
      
      err = unlink (fqn);
      if (err == -1)
        {
          fprintf (stderr, "%s: could not unlink file '%s': %s\n",
                   __FUNCTION__, fqn, strerror (errno));
          closedir (dirstream);
	  free (dir);
          return;
        }
    }
  closedir (dirstream);
  
  err = rmdir (dir);
  if (err == -1)
    {
      fprintf (stderr, "%s: could not remove directroy '%s': %s\n",
               __FUNCTION__, dir, strerror (errno));
    }

  free (dir);
}


/* EOF */

