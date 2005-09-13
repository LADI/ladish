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

#ifndef __LASHD_CLIENT_H__
#define __LASHD_CLIENT_H__

#include <uuid/uuid.h>
#include <libxml/tree.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "store.h"

#define CAST_BAD (char *)

typedef struct _client client_t;

struct _client
{
  uuid_t                id;
  enum LASH_Client_Flag  flags;
  char                 *class;
  char                 *working_dir;
  char                 *requested_project;
  int                   argc;
  char                **argv;

  unsigned long         conn_id;
  char *                name;
  store_t *             store;
  
  char *                jack_client_name;
  lash_list_t *          jack_patches;
  unsigned char         alsa_client_id;
  lash_list_t *          alsa_patches;

  struct _project      *project;
};

#define CLIENT_CONFIG_DATA_SET(x)   (((x)->flags) & LASH_Config_Data_Set)
#define CLIENT_CONFIG_FILE(x)       (((x)->flags) & LASH_Config_File)
#define CLIENT_SERVER_INTERFACE(x)  (((x)->flags) & LASH_Server_Interface)
#define CLIENT_NO_AUTORESUME(x)     (((x)->flags) & LASH_No_Autoresume)
#define CLIENT_TERMINAL(x)          (((x)->flags) & LASH_Terminal)
#define CLIENT_SAVED(x)             (((x)->flags) & LASH_Saved)


client_t * client_new ();
void       client_destroy (client_t * client);

const char * client_get_id_str   (client_t * client);
const char * client_get_identity (client_t * client);

void client_set_from_connect_params (client_t *client, lash_connect_params_t *params);
void client_set_name              (client_t * client, const char * name);
void client_set_conn_id           (client_t * client, unsigned long id);
void client_set_jack_client_name  (client_t * client, const char * name);
void client_set_alsa_client_id    (client_t * client, unsigned char id);
void client_set_class             (client_t * client, const char * class);
void client_set_working_dir       (client_t * client, const char * working_dir);
void client_set_requested_project (client_t * client, const char * requested_project);
void client_set_id                (client_t * client, uuid_t id);

void client_generate_id          (client_t * client);
/*void client_restore_jack_patches (client_t * client);
void client_restore_alsa_patches (client_t * client); */

int             client_store_open       (client_t * client, const char * dir);
lash_config_t *  client_store_get_config (client_t * client, const char * key);
int             client_store_write      (client_t * client);
int             client_store_close      (client_t * client);

void client_parse_xml (client_t * client, xmlNodePtr parent);

#endif /* __LASHD_CLIENT_H__ */
