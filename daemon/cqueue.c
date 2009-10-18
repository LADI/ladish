/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the command queue
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

#include "cmd.h"

void ladish_cqueue_init(struct ladish_cqueue * queue_ptr)
{
  queue_ptr->cancel = false;
  INIT_LIST_HEAD(&queue_ptr->queue);
}

void ladish_cqueue_run(struct ladish_cqueue * queue_ptr)
{
  struct list_head * node_ptr;
  struct ladish_command * cmd_ptr;

loop:
  if (list_empty(&queue_ptr->queue))
  {
    return;
  }

  node_ptr = queue_ptr->queue.next;
  cmd_ptr = list_entry(node_ptr, struct ladish_command, siblings);

  ASSERT(cmd_ptr->run != NULL);
  ASSERT(cmd_ptr->state == LADISH_COMMAND_STATE_PENDING || cmd_ptr->state == LADISH_COMMAND_STATE_WAITING);

  if (!cmd_ptr->run(cmd_ptr->context))
  {
    ladish_cqueue_uninit(queue_ptr);
    return;
  }

  switch (cmd_ptr->state)
  {
  case LADISH_COMMAND_STATE_DONE:
    break;
  case LADISH_COMMAND_STATE_WAITING:
    return;
  default:
    log_error("unexpected cmd state %u after run()", cmd_ptr->state);
    ASSERT_NO_PASS;
    ladish_cqueue_uninit(queue_ptr);
    return;
  }

  list_del(node_ptr);

  if (cmd_ptr->destructor != NULL)
  {
    cmd_ptr->destructor(cmd_ptr->context);
  }

  free(cmd_ptr);

  if (queue_ptr->cancel && list_empty(&queue_ptr->queue));
  {
    queue_ptr->cancel = false;
  }

  goto loop;
}

void ladish_cqueue_cancel(struct ladish_cqueue * queue_ptr)
{
  struct list_head * node_ptr;
  struct ladish_command * cmd_ptr;

  if (list_empty(&queue_ptr->queue))
  { /* nothing to cancel */
    return;
  }

  node_ptr = queue_ptr->queue.prev;
  cmd_ptr = list_entry(node_ptr, struct ladish_command, siblings);
  if (cmd_ptr->state != LADISH_COMMAND_STATE_WAITING)
  {
    ladish_cqueue_uninit(queue_ptr);
    return;
  }

  /* clear all commands except currently waiting one */
  while (queue_ptr->queue.next != queue_ptr->queue.prev)
  {
    node_ptr = queue_ptr->queue.prev;
    list_del(node_ptr);

    cmd_ptr = list_entry(node_ptr, struct ladish_command, siblings);

    if (cmd_ptr->destructor != NULL)
    {
      cmd_ptr->destructor(cmd_ptr->context);
    }

    free(cmd_ptr);
  }

  queue_ptr->cancel = true;
  cmd_ptr->cancel = true;
  ladish_cqueue_run(queue_ptr);
}

bool ladish_cqueue_add_command(struct ladish_cqueue * queue_ptr, struct ladish_command * cmd_ptr)
{
  ASSERT(cmd_ptr->run != NULL);

  if (queue_ptr->cancel)
  {
    return false;
  }

  list_add_tail(&cmd_ptr->siblings, &queue_ptr->queue);
  ladish_cqueue_run(queue_ptr);
  return true;
}

void ladish_cqueue_uninit(struct ladish_cqueue * queue_ptr)
{
  struct list_head * node_ptr;
  struct ladish_command * cmd_ptr;

  while (!list_empty(&queue_ptr->queue))
  {
    node_ptr = queue_ptr->queue.next;
    list_del(node_ptr);

    cmd_ptr = list_entry(node_ptr, struct ladish_command, siblings);

    if (cmd_ptr->destructor != NULL)
    {
      cmd_ptr->destructor(cmd_ptr->context);
    }

    free(cmd_ptr);
  }

  queue_ptr->cancel = false;
}

void * ladish_command_new(size_t size)
{
  struct ladish_command * cmd_ptr;

  ASSERT(size >= sizeof(struct ladish_command));

  cmd_ptr = malloc(size);
  if (cmd_ptr == NULL)
  {
    log_error("malloc failed to allocate command with size %zu", size);
    return NULL;
  }

  cmd_ptr->state = LADISH_COMMAND_STATE_PREPARE;
  cmd_ptr->cancel = false;

  cmd_ptr->context = cmd_ptr;
  cmd_ptr->run = NULL;
  cmd_ptr->destructor = NULL;

  return cmd_ptr;
}
