/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include <stdbool.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <libxml/tree.h>
#include <sys/types.h>
#include <dbus/dbus.h>

#include "common/klist.h"

#include "lash/types.h"
#include "common/list.h"

#include "types.h"

struct _client
{
	struct list_head        siblings;

	uuid_t                  id;
	char                    id_str[37];
	pid_t                   pid;
	enum LASH_Client_Flag   flags;
	char                   *class;
	char                   *working_dir;
	char                   *data_path;
	int                     argc;
	char                  **argv;

	char                   *dbus_name;
	char                   *name;
	store_t                *store;

	dbus_uint64_t           pending_task;
	enum LASH_Event_Type    task_type;
	uint8_t                 task_progress;

	char                   *jack_client_name;
	lash_list_t *           jack_patches;
	unsigned char           alsa_client_id;
	lash_list_t            *alsa_patches;

	struct list_head        dependencies;
	struct list_head        unsatisfied_deps;

	project_t              *project;
};

enum
{
	LASH_Saved = 0x01000000  /* Client has been saved */
};

#define CLIENT_CONFIG_DATA_SET(x)    (((x)->flags) & LASH_Config_Data_Set)
#define CLIENT_CONFIG_FILE(x)        (((x)->flags) & LASH_Config_File)
#define CLIENT_HAS_INTERNAL_STATE(x) (((x)->flags) & (LASH_Config_Data_Set | LASH_Config_File))
#define CLIENT_SERVER_INTERFACE(x)   (((x)->flags) & LASH_Server_Interface)
#define CLIENT_NO_AUTORESUME(x)      (((x)->flags) & LASH_No_Autoresume)
#define CLIENT_TERMINAL(x)           (((x)->flags) & LASH_Terminal)
#define CLIENT_SAVED(x)              (((x)->flags) & LASH_Saved)

client_t *
client_new(void);

void
client_destroy(client_t *client);

const char *
client_get_identity(client_t *client);

void
client_disconnected(client_t *client);

bool
client_store_open(client_t   *client,
                  const char *dir);

bool
client_store_close(client_t *client);

void
client_task_progressed(client_t *client,
                       uint8_t   percentage);

void
client_task_completed(client_t *client,
                      bool      was_succesful);

void
client_parse_xml(project_t  *project,
                 client_t   *client,
                 xmlNodePtr  parent);

#endif /* __LASHD_CLIENT_H__ */
