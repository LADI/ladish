/*
 *   LASH
 *
 *   Copyright (C) 2003 Robert Ham <rah@bash.sh>
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

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <uuid/uuid.h>

#include "lash/lash.h"
#include "common/list.h"

typedef struct _client client_t;

struct _client
{
  uuid_t        id;
  char *        name;
  char *        jack_client_name;
  unsigned char alsa_client_id;
};

client_t * client_new (uuid_t id);
void       client_destroy (client_t * client);

const char * client_get_identity (client_t * client);

#endif /* __CLIENT_H__ */
