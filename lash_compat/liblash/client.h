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

#ifndef __LIBLASH_CLIENT_H__
#define __LIBLASH_CLIENT_H__

#include <stdint.h>
#include <uuid/uuid.h>
#include <dbus/dbus.h>

#include "config.h"

#include "../../dbus/service.h"
#include "../../dbus/method.h"

#include "lash/types.h"

#ifdef LASH_OLD_API
# include "../../common/klist.h"
#endif

struct _lash_client
{
	char       *class;
	uuid_t      id;
	char       *name; // TODO: Get name from server
	char       *project_name;
	uint8_t     is_controller;
	int         argc;
	char      **argv;
	char       *working_dir;
	int         flags;
	service_t  *dbus_service;
	bool        quit; // TODO: What to do with this?
	uint64_t    pending_task;
	uint8_t     task_progress;
	short       server_connected;
	char       *data_path;

	struct
	{
		LashEventCallback    trysave;
		LashEventCallback    save;
		LashEventCallback    load;
		LashEventCallback    quit;
		LashEventCallback    name;
		LashEventCallback    proj;
		LashEventCallback    path;
		LashConfigCallback   save_data_set;
		LashConfigCallback   load_data_set;
		LashControlCallback  control;
	} cb;

	struct
	{
		void                *trysave;
		void                *save;
		void                *load;
		void                *quit;
		void                *name;
		void                *proj;
		void                *path;
		void                *save_data_set;
		void                *load_data_set;
		void                *control;
	} ctx;

#ifdef LASH_OLD_API
	struct list_head events_in;
	uint32_t         num_events_in;

	struct list_head configs_in;
	uint32_t         num_configs_in;

	method_msg_t     unsent_configs;
	DBusMessageIter  iter, array_iter;
#endif
};

lash_client_t *
lash_client_new(void);

void
lash_client_destroy(lash_client_t *client);

#ifdef LASH_OLD_API
void
lash_client_add_event(lash_client_t *client,
                      lash_event_t  *event);

void
lash_client_add_config(lash_client_t *client,
                       lash_config_t *config);
#endif

#endif /* __LIBLASH_CLIENT_H__ */
