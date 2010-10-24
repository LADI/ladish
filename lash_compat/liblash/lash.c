/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the liblash implementaiton
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

#include "lash/lash.h"
#include "../../common/catdup.h"

const char * lash_protocol_string(lash_protocol_t protocol)
{
  return "ladish";
}

lash_client_t * lash_client_open(const char * class, int flags, int argc, char ** argv)
{
	return NULL;
}

void lash_jack_client_name(lash_client_t * client, const char * name)
{
	return;
}

void lash_alsa_client_id(lash_client_t * client, unsigned char id)
{
	return;
}

lash_args_t * lash_extract_args(int * argc, char *** argv)
{
	return NULL;
}

void lash_args_destroy(lash_args_t * args)
{
}

lash_client_t * lash_init(const lash_args_t * args, const char * class, int client_flags, lash_protocol_t protocol)
{
  return NULL;
}

unsigned int lash_get_pending_event_count(lash_client_t * client)
{
  return 0;
}

unsigned int lash_get_pending_config_count(lash_client_t * client)
{
  return 0;
}

lash_event_t * lash_get_event(lash_client_t * client)
{
  return NULL;
}

lash_config_t * lash_get_config(lash_client_t * client)
{
  return NULL;
}

void lash_send_event(lash_client_t * client, lash_event_t * event)
{
}

void lash_send_config(lash_client_t * client, lash_config_t * config)
{
}

int lash_server_connected(lash_client_t * client)
{
  return 0;
}

const char * lash_get_server_name(lash_client_t * client)
{
	return NULL;
}

lash_event_t * lash_event_new(void)
{
  return NULL;
}

lash_event_t * lash_event_new_with_type(enum LASH_Event_Type type)
{
  return NULL;
}

lash_event_t * lash_event_new_with_all (enum LASH_Event_Type type, const char * string)
{
  return NULL;
}

void lash_event_destroy(lash_event_t * event)
{
}

enum LASH_Event_Type lash_event_get_type(const lash_event_t * event)
{
  return 0;
}

const char * lash_event_get_string(const lash_event_t * event)
{
  return NULL;
}

const char * lash_event_get_project(const lash_event_t * event)
{
  return NULL;
}

void lash_event_get_client_id(const lash_event_t * event, uuid_t id)
{
}
 
void lash_event_set_type(lash_event_t * event, enum LASH_Event_Type type)
{
}

void lash_event_set_string(lash_event_t * event, const char * string)
{
}

void lash_event_set_project(lash_event_t * event, const char * project)
{
}

void lash_event_set_client_id(lash_event_t * event, uuid_t id)
{
}

void lash_event_set_alsa_client_id(lash_event_t * event, unsigned char alsa_id)
{
}

unsigned char lash_event_get_alsa_client_id(const lash_event_t * event)
{
  return 0;
}

void lash_str_set_alsa_client_id(char * str, unsigned char alsa_id)
{
}

unsigned char lash_str_get_alsa_client_id(const char * str)
{
  return 0;
}

lash_config_t * lash_config_new(void)
{
  return NULL;
}

lash_config_t * lash_config_dup(const lash_config_t * config)
{
  return NULL;
}

lash_config_t * lash_config_new_with_key(const char * key)
{
  return NULL;
}

void lash_config_destroy(lash_config_t * config)
{
}

const char * lash_config_get_key(const lash_config_t * config)
{
  return NULL;
}

const void * lash_config_get_value(const lash_config_t * config)
{
  return NULL;
}

size_t lash_config_get_value_size(const lash_config_t * config)
{
  return 0;
}

void lash_config_set_key(lash_config_t * config, const char * key)
{
}

void lash_config_set_value(lash_config_t * config, const void * value, size_t value_size)
{
}

uint32_t lash_config_get_value_int(const lash_config_t * config)
{
  return 0;
}

float lash_config_get_value_float(const lash_config_t * config)
{
  return 0.0;
}

double lash_config_get_value_double(const lash_config_t * config)
{
  return 0.0;
}

const char * lash_config_get_value_string(const lash_config_t * config)
{
  return NULL;
}

void lash_config_set_value_int(lash_config_t * config, uint32_t value)
{
}

void lash_config_set_value_float(lash_config_t * config, float value)
{
}

void lash_config_set_value_double(lash_config_t * config, double value)
{
}

void lash_config_set_value_string(lash_config_t * config, const char * value)
{
}

const char * lash_get_fqn(const char * dir, const char * file)
{
  static char * fqn = NULL;

  if (fqn != NULL)
  {
    free(fqn);
  }

  fqn = catdup3(dir, "/", file);

  return fqn;
}
