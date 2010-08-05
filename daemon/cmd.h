/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the command queue stuff
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

#ifndef CMD_H__28542C9B_7CB8_40F8_BBB6_DCE13CBB1E7F__INCLUDED
#define CMD_H__28542C9B_7CB8_40F8_BBB6_DCE13CBB1E7F__INCLUDED

#include "common.h"

#define LADISH_COMMAND_STATE_PREPARE     0
#define LADISH_COMMAND_STATE_PENDING     1
#define LADISH_COMMAND_STATE_WAITING     2
#define LADISH_COMMAND_STATE_DONE        3

struct ladish_command
{
  struct list_head siblings;

  unsigned int state;
  bool cancel;

  void * context;
  bool (* run)(void * context);
  void (* destructor)(void * context);
};

struct ladish_cqueue
{
  bool cancel;
  struct list_head queue;
};

void ladish_cqueue_init(struct ladish_cqueue * queue_ptr);
void ladish_cqueue_run(struct ladish_cqueue * queue_ptr);
void ladish_cqueue_cancel(struct ladish_cqueue * queue_ptr);
bool ladish_cqueue_add_command(struct ladish_cqueue * queue_ptr, struct ladish_command * command_ptr);
void ladish_cqueue_drop_command(struct ladish_cqueue * queue_ptr);
void ladish_cqueue_clear(struct ladish_cqueue * queue_ptr);

void * ladish_command_new(size_t size);

bool ladish_command_new_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name);
bool ladish_command_load_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name);
bool ladish_command_rename_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name);
bool ladish_command_save_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * new_studio_name);
bool ladish_command_start_studio(void * call_ptr, struct ladish_cqueue * queue_ptr);
bool ladish_command_stop_studio(void * call_ptr, struct ladish_cqueue * queue_ptr);
bool ladish_command_unload_studio(void * call_ptr, struct ladish_cqueue * queue_ptr);
bool ladish_command_exit(void * call_ptr, struct ladish_cqueue * queue_ptr);

bool
ladish_command_new_app(
  void * call_ptr,
  struct ladish_cqueue * queue_ptr,
  const char * opath,
  bool terminal,
  const char * commandline,
  const char * name,
  uint8_t level);

bool ladish_command_change_app_state(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * opath, uint64_t id, unsigned int target_state);
bool ladish_command_remove_app(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * opath, uint64_t id);
bool ladish_command_create_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name, const char * template_name);
bool ladish_command_delete_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name);

bool
ladish_command_save_project(
  void * call_ptr,
  struct ladish_cqueue * queue_ptr,
  const uuid_t room_uuid_ptr,
  const char * project_dir,
  const char * project_name);

#endif /* #ifndef CMD_H__28542C9B_7CB8_40F8_BBB6_DCE13CBB1E7F__INCLUDED */
