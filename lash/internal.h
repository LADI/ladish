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

#ifndef __LASH_INTERNAL_H__
#define __LASH_INTERNAL_H__

#include <uuid/uuid.h>

#include <lash/types.h>

#define set_string_property(property, value) \
  \
  if (property) \
    free (property); \
  \
  if (value) \
    (property) = lash_strdup (value); \
  else \
    (property) = NULL;


typedef struct _lash_comm_event lash_comm_event_t;
typedef struct _lash_comm lash_comm_t;

enum LASH_Internal_Client_Flag
  {
    LASH_Saved              =  0x01000000  /* client has been saved */
  };

struct _lash_config
{
  char * key;
  void * value;
  size_t value_size;
};

struct _lash_event
{
  enum LASH_Event_Type  type;
  char                *string;
  char                *project;
  uuid_t               client_id;
};

struct _lash_exec_params
{
  int    flags;
  int    argc;
  char **argv;
  char  *working_dir;
  char  *server;
  char  *project;
  uuid_t id;
};

struct _loader
{
  int   server_socket;
  int   loader_socket;
  pid_t loader_pid;
};

/*snd_seq_port_subscribe_t * lash_event_get_string_alsa_patch (lash_event_t * event);
lash_jack_patch_t *         lash_event_get_string_jack_patch (lash_event_t * event); */

void lash_config_init (struct _lash_config * config);
void lash_config_free (struct _lash_config * config);

void lash_event_init (struct _lash_event * event);
void lash_event_free (struct _lash_event * event);



#endif /* __LASH_INTERNAL_H__ */
