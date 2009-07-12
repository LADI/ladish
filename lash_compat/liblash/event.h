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

#ifndef __LIBLASH_EVENT_H__
#define __LIBLASH_EVENT_H__

#include <uuid/uuid.h>

#include "common/klist.h"
#include "lash/types.h"

typedef void (*LASHEventConstructor) (lash_client_t *, lash_event_t *);

struct _lash_event
{
  struct list_head      siblings;
  enum LASH_Event_Type  type;
  char                 *string;
  char                 *project;
  uuid_t                client_id;
  LASHEventConstructor  ctor;
};

#endif /* __LIBLASH_EVENT_H__ */
