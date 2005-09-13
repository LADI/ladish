/*
 *   LASH
 *    
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

#ifndef __LASH_SERVER_CLIENT_H__
#define __LASH_SERVER_CLIENT_H__

#include <stdint.h>
#include <uuid/uuid.h>

#include <ladcca/ladcca.h>

#define LASH_CONFIGURED_WITH_DATA_SET(x)  ((x) & LASH_Config_Data_Set)
#define LASH_CONFIGURED_WITH_FILE(x)      ((x) & LASH_Config_File)
#define LASH_IS_SERVER_INTERFACEE(x)      ((x) & LASH_Server_Interface) 
#define LASH_NO_AUTORESUME(x)             ((x) & LASH_No_Autoresume)
#define LASH_RUNS_IN_TERMINAL(x)          ((x) & LASH_Terminal)

#define LASH_CLIENT_CONFIGURED_WITH_DATA_SET(x)  ((lash_client_get_flags (x)) & LASH_Config_Data_Set)
#define LASH_CLIENT_CONFIGURED_WITH_FILE(x)      ((lash_client_get_flags (x)) & LASH_Config_File)
#define LASH_CLIENT_IS_SERVER_INTERFACE(x)       ((lash_client_get_flags (x)) & LASH_Server_Interface)
#define LASH_CLIENT_NO_AUTORESUME(x)             ((lash_client_get_flags (x)) & LASH_No_Autoresume)
#define LASH_CLIENT_RUNS_IN_TERMINAL(x)          ((lash_client_get_flags (x)) & LASH_Terminal)

struct _lash_client {
  char *            project_name;
  
  int               flags;
  int               argc;
  char            **argv;
  char             *working_dir;
  uuid_t            id;
  char             *class;
  
  struct _lash_comm *comm;
  
  /* this is used for the server name and port in the server-side loader */
  char             *server_name;

  /* this is also used for the protocol version in Connect events */
  uint32_t server_connected;
};

void lash_client_init (lash_client_t * client);
void lash_client_free (lash_client_t * client);

lash_client_t * lash_client_new ();
void           lash_client_destroy (lash_client_t *);
lash_client_t * lash_client_dup     (const lash_client_t *);

const char *                 lash_client_get_project_name (const lash_client_t * client);
enum LASH_Client_Flags        lash_client_get_flags (const lash_client_t * client);
const char *                 lash_client_get_working_dir (const lash_client_t * client);
int                          lash_client_get_argc (const lash_client_t * client);
char **                      lash_client_get_argv (const lash_client_t * client);
struct _lash_comm *           lash_client_get_comm (const lash_client_t * client);
const char *                 lash_client_get_server_name (const lash_client_t * client);
void                         lash_client_get_id (const lash_client_t * client, uuid_t id_copy);
const char *                 lash_client_get_id_str (const lash_client_t * client);
const char *                 lash_client_get_class (const lash_client_t * client);
uint32_t                     lash_client_get_server_connected (const lash_client_t * client);

void lash_client_set_project_name (lash_client_t * client, const char * project);
void lash_client_set_flags (lash_client_t * client, enum LASH_Client_Flags);
void lash_client_set_working_dir (lash_client_t * client, const char * working_dir);
void lash_client_set_args (lash_client_t * client, int argc, char ** argv);
void lash_client_set_comm (lash_client_t * client, struct _lash_comm * comm);
void lash_client_set_server_name (lash_client_t * client, const char * name);
void lash_client_set_id (lash_client_t * client, uuid_t id);
void lash_client_set_class (lash_client_t * client, const char * class);
void lash_client_set_server_connected (lash_client_t * client, uint32_t connected);

#endif /* __LASH_SERVER_CLIENT_H__ */
