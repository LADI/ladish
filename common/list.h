/*
    Based on gslist.c from glib-1.2.9 (LGPL).
 
    Adaption to JACK, Copyright (C) 2002 Kai Vehmanen.
      - replaced use of gtypes with normal ANSI C types
      - glib's memery allocation routines replaced with
        malloc/free calls
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser Lesser General Public License
    along with this program; if not, write to the Free Software 
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

    $Id: list.h,v 1.1 2005-09-13 05:22:59 drobilla Exp $
*/

/*
 * Nicked from jack, with:
 *   s/jack_slist/lash_list/g
 *   s/JSList/lash_list_t/g
 *   s/malloc/lash_malloc/g
 * - Bob Ham
 */

#ifndef __LASH_LIST_H__
#define __LASH_LIST_H__

#include <stdlib.h>

#include "common/safety.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _lash_list lash_list_t;

typedef int		(*JCompareFunc)		(void*	a,
						 void*	b);
struct _lash_list
{
  void *data;
  lash_list_t *next;
};

static __inline__
lash_list_t*
lash_list_alloc (void)
{
  lash_list_t *new_list;

  new_list = (lash_list_t *) lash_malloc(1, sizeof(lash_list_t));
  new_list->data = NULL;
  new_list->next = NULL;

  return new_list;
}

static __inline__ 
lash_list_t*
lash_list_prepend (lash_list_t   *list,
		    void     *data)
{
  lash_list_t *new_list;

  new_list = (lash_list_t *) lash_malloc(1, sizeof(lash_list_t));
  new_list->data = data;
  new_list->next = list;

  return new_list;
}

#define lash_list_next(slist)	((slist) ? (((lash_list_t *)(slist))->next) : NULL)
static __inline__ 
lash_list_t*
lash_list_last (lash_list_t *list)
{
  if (list)
    {
      while (list->next)
	list = list->next;
    }

  return list;
}

static __inline__ 
lash_list_t*
lash_list_remove_link (lash_list_t *list,
                       lash_list_t *llink)
{
  lash_list_t *tmp;
  lash_list_t *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp == llink)
	{
	  if (prev)
	    prev->next = tmp->next;
	  if (list == tmp)
	    list = list->next;

	  tmp->next = NULL;
	  break;
	}

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

static __inline__ 
void
lash_list_free (lash_list_t *list)
{
  while (list)
    {
	    lash_list_t *next = list->next;
	    free(list);
	    list = next;
    }
}

static __inline__ 
void
lash_list_free_1 (lash_list_t *list)
{
  if (list)
    {
      free(list);
    }
}

static __inline__ 
lash_list_t*
lash_list_remove (lash_list_t   *list,
		   void     *data)
{
  lash_list_t *tmp;
  lash_list_t *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp->data == data)
	{
	  if (prev)
	    prev->next = tmp->next;
	  if (list == tmp)
	    list = list->next;

	  tmp->next = NULL;
	  lash_list_free (tmp);

	  break;
	}

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

static __inline__ 
unsigned int
lash_list_length (lash_list_t *list)
{
  unsigned int length = 0;

  while (list)
    {
      length++;
      list = list->next;
    }

  return length;
}

static __inline__ 
lash_list_t*
lash_list_find (lash_list_t   *list,
		 void     *data)
{
  while (list)
    {
      if (list->data == data)
	break;
      list = list->next;
    }

  return list;
}

static __inline__ 
lash_list_t*
lash_list_copy (lash_list_t *list)
{
  lash_list_t *new_list = NULL;

  if (list)
    {
      lash_list_t *last;

      new_list = lash_list_alloc ();
      new_list->data = list->data;
      last = new_list;
      list = list->next;
      while (list)
	{
	  last->next = lash_list_alloc ();
	  last = last->next;
	  last->data = list->data;
	  list = list->next;
	}
    }

  return new_list;
}

static __inline__ 
lash_list_t*
lash_list_append (lash_list_t   *list,
		   void     *data)
{
  lash_list_t *new_list;
  lash_list_t *last;

  new_list = lash_list_alloc ();
  new_list->data = data;

  if (list)
    {
      last = lash_list_last (list);
      last->next = new_list;

      return list;
    }
  else
      return new_list;
}

static __inline__ 
lash_list_t* 
lash_list_sort_merge  (lash_list_t      *l1, 
			lash_list_t      *l2,
			JCompareFunc compare_func)
{
  lash_list_t list, *l;

  l=&list;

  while (l1 && l2)
    {
      if (compare_func(l1->data,l2->data) < 0)
        {
	  l=l->next=l1;
	  l1=l1->next;
        } 
      else 
	{
	  l=l->next=l2;
	  l2=l2->next;
        }
    }
  l->next= l1 ? l1 : l2;
  
  return list.next;
}

static __inline__ 
lash_list_t* 
lash_list_sort (lash_list_t       *list,
		 JCompareFunc compare_func)
{
  lash_list_t *l1, *l2;

  if (!list) 
    return NULL;
  if (!list->next) 
    return list;

  l1 = list; 
  l2 = list->next;

  while ((l2 = l2->next) != NULL)
    {
      if ((l2 = l2->next) == NULL) 
	break;
      l1=l1->next;
    }
  l2 = l1->next; 
  l1->next = NULL;

  return lash_list_sort_merge (lash_list_sort (list, compare_func),
				lash_list_sort (l2,   compare_func),
				compare_func);
}

static __inline__ 
lash_list_t *
lash_list_concat (lash_list_t *list1, lash_list_t *list2)
{
  if (list2)
    {
      if (list1)
        lash_list_last (list1)->next = list2;
      else
        list1 = list2;
    }

  return list1;
}


#ifdef __cplusplus
}
#endif


#endif /* __LASH_LIST_H__ */
