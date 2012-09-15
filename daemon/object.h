/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the generic object system functionality
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef OBJECT_H__897CDBE2_6438_4C66_8B39_0B4D4B3CE3EF__INCLUDED
#define OBJECT_H__897CDBE2_6438_4C66_8B39_0B4D4B3CE3EF__INCLUDED

#define LADISH_OBJECT_SIGNATURE 0x08032004

typedef void (* ladish_object_destructor)(void *);

struct ladish_object
{
  unsigned int sig;
  unsigned int refcount;
  ladish_object_destructor destructor;
};

typedef struct ladish_object ladish_object;

#define LADISH_OBJECT_CHECK(obj_ptr) ASSERT(obj_ptr->sig == LADISH_OBJECT_SIGNATURE)

#define obj_ptr ((struct ladish_object *)obj)

static inline void ladish_object_init(void * obj, unsigned int refcount, ladish_object_destructor destructor)
{
  obj_ptr->sig = LADISH_OBJECT_SIGNATURE;
  obj_ptr->refcount = refcount;
  obj_ptr->destructor = destructor;
}

static inline void ladish_add_ref(void * obj)
{
  LADISH_OBJECT_CHECK(obj_ptr);
  obj_ptr->refcount++;
}

static inline void ladish_del_ref(void * obj)
{
  LADISH_OBJECT_CHECK(obj_ptr);
  ASSERT(obj_ptr->refcount > 0);
  obj_ptr->refcount--;
  if (obj_ptr->refcount == 0)
  {
    obj_ptr->destructor(obj);
  }
}

#undef obj_ptr

#endif /* #ifndef OBJECT_H__897CDBE2_6438_4C66_8B39_0B4D4B3CE3EF__INCLUDED */
