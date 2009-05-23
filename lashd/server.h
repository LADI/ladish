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

#ifndef __LASHD_SERVER_H__
#define __LASHD_SERVER_H__

#include "../config.h"

#include <stdbool.h>
#include <sys/types.h>
#include <dbus/dbus.h>
#include <uuid/uuid.h>

#include "types.h"
#include "dbus/service.h"
#include "common/klist.h"

#ifdef HAVE_ALSA
# include "alsa_mgr.h"
#endif

extern server_t *g_server;

struct _server
{
	service_t            *dbus_service;
#ifdef HAVE_JACK_DBUS
	lashd_jackdbus_mgr_t *jackdbus_mgr;
#else
	jack_mgr_t           *jack_mgr;
#endif
#ifdef HAVE_ALSA
	alsa_mgr_t           *alsa_mgr;
#else
	void                 *alsa_mgr;
#endif

	char                 *projects_dir;
	struct list_head      loaded_projects;
	struct list_head      all_projects;
	struct list_head      appdb;
	dbus_uint64_t         task_iter;

	bool                  quit;
};

bool
server_start(const char *default_dir);

void
server_stop(void);

void
server_main(void);

project_t *
server_find_project_by_name(const char *project_name);

client_t *
server_add_client(const char  *dbus_name,
                  pid_t        pid,
                  const char  *class,
                  int          flags,
                  const char  *working_dir,
                  int          argc,
                  char       **argv);

client_t *
server_find_client_by_dbus_name(const char *dbus_name);

client_t *
server_find_client_by_pid(pid_t pid);

client_t *
server_find_lost_client_by_pid(pid_t pid);

client_t *
server_find_client_by_id(uuid_t id);

void
server_close_project(project_t *project);

void
server_save_all_projects(void);

void
server_close_all_projects(void);

bool
server_project_close_by_name(const char *project_name);

bool
server_project_restore_by_dir(const char *directory);

bool
server_project_restore_by_name(const char *project_name);

bool
server_project_save_by_name(const char *project_name);

#endif /* __LASHD_SERVER_H__ */
