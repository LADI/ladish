/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   Python bindings for LASH
 *    
 *   Copyright (C) 2006 Nedko Arnaudov <nedko@arnaudov.name>
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
 *
 *****************************************************************************/

#ifndef LASH_I__74601D15_FF3A_4086_B6F6_8D626BBCD7A2__INCLUDED
#define LASH_I__74601D15_FF3A_4086_B6F6_8D626BBCD7A2__INCLUDED

#ifdef SWIG
%module lash

%typemap(in) (int *argc, char ***argv)
{
  int i;

  if (!PyList_Check($input))
  {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    goto fail;
  }

  $1 = (int *) malloc(sizeof(int));
  if ($1 == NULL)
  {
    PyErr_SetString(PyExc_ValueError, "malloc() failed.");
    goto fail;
  }

  *$1 = PyList_Size($input);

  $2 = (char ***) malloc(sizeof(char **));
  if ($2 == NULL)
  {
    PyErr_SetString(PyExc_ValueError, "malloc() failed.");
    goto fail;
  }

  *$2 = (char **) malloc((*$1+1)*sizeof(char *));
  if (*$2 == NULL)
  {
    PyErr_SetString(PyExc_ValueError, "malloc() failed.");
    goto fail;
  }

  for (i = 0; i < *$1; i++)
  {
    PyObject *s = PyList_GetItem($input,i);
    if (!PyString_Check(s))
    {
        PyErr_SetString(PyExc_ValueError, "List items must be strings");
        goto fail;
    }

    (*$2)[i] = PyString_AsString(s);
  }

  (*$2)[i] = 0;
}

%typemap(argout) (int *argc, char ***argv)
{
  int i;

  for (i = 0; i < *$1; i++)
  {
    PyObject *s = PyString_FromString((*$2)[i]);
    PyList_SetItem($input,i,s);
  }

  while (i < PySequence_Size($input))
  {
    PySequence_DelItem($input,i);
  }
}

%typemap(freearg) (int *argc, char ***argv)
{
   if ($1 != NULL)
   {
     //printf("freeing argc pointer storage\n");
     free($1);
     $1 = NULL;
   }

   if ($2 != NULL)
   {
     if (*$2 != NULL)
     {
       //printf("freeing array pointed by argv pointer storage\n");
       free(*$2);
     }

     //printf("freeing argv pointer storage\n");
     free($2);
     $1 = NULL;
   }
}

%{
#include <lash/client_interface.h>
#include <lash/types.h>
#include <lash/event.h>
#include <lash/config.h>
#include "lash.h"
typedef unsigned char * uuid_t_compat;
#define uuid_t uuid_t_compat
%}

#endif

%include <lash/client_interface.h>
%include <lash/types.h>
%include <lash/event.h>
%include <lash/config.h>
%include <lash.h>

#endif /* #ifndef LASH_I__74601D15_FF3A_4086_B6F6_8D626BBCD7A2__INCLUDED */
