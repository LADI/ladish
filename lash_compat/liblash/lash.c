/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
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

#define LADISH_DEBUG

#include <arpa/inet.h>

#include "lash/lash.h"
#include "../../common.h"
#include "../../common/catdup.h"
//#define LOG_OUTPUT_STDOUT
#include "../../log.h"

struct _lash_client
{
  int flags;
};

static struct _lash_client g_client;

struct _lash_event
{
  enum LASH_Event_Type type;
  char * string;
};

struct _lash_config
{
  char * key;
  size_t size;
  void * value;
};

const char * lash_protocol_string(lash_protocol_t protocol)
{
  return "ladish";
}

lash_args_t * lash_extract_args(int * argc, char *** argv)
{
  /* nothing to do, ladish does not pass any specific arguments */
	return NULL;
}

void lash_args_destroy(lash_args_t * args)
{
  /* nothing to do, ladish does not pass any specific arguments */
}

lash_client_t * lash_init(const lash_args_t * args, const char * class, int client_flags, lash_protocol_t protocol)
{
  if ((client_flags & LASH_Server_Interface) != 0)
  {
    log_error("ladish does not implement LASH server interface.");
    return NULL;
  }

  log_debug("ladish LASH support initialized (%s %s)", (client_flags & LASH_Config_File) != 0 ? "file" : "", (client_flags & LASH_Config_Data_Set) != 0 ? "dict" : "");
  g_client.flags = client_flags;

  return &g_client;
}

unsigned int lash_get_pending_event_count(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  /* TODO */
  return 0;
}

unsigned int lash_get_pending_config_count(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  /* TODO */
  return 0;
}

lash_event_t * lash_get_event(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  /* TODO */
  return NULL;
}

lash_config_t * lash_get_config(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  /* TODO */
  return NULL;
}

void lash_send_event(lash_client_t * client_ptr, lash_event_t * event_ptr)
{
  ASSERT(client_ptr == &g_client);

  log_debug("lash_send_event() called. type=%d string=%s", event_ptr->type, event_ptr->string != NULL ? event_ptr->string : "(NULL)");

  /* TODO */

  lash_event_destroy(event_ptr);
}

void lash_send_config(lash_client_t * client_ptr, lash_config_t * config_ptr)
{
  ASSERT(client_ptr == &g_client);

  log_debug("lash_send_config() called. key=%s value_size=%zu", config_ptr->key, config_ptr->size);

  /* TODO */

  lash_config_destroy(config_ptr);
}

int lash_server_connected(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  return 1;                     /* yes */
}

const char * lash_get_server_name(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
	return "localhost";
}

lash_event_t * lash_event_new(void)
{
  struct _lash_event * event_ptr;

  event_ptr = malloc(sizeof(struct _lash_event));
  if (event_ptr == NULL)
  {
    log_error("malloc() failed to allocate lash event struct");
    return NULL;
  }

  event_ptr->type = 0;
  event_ptr->string = NULL;

  return event_ptr;
}

lash_event_t * lash_event_new_with_type(enum LASH_Event_Type type)
{
	lash_event_t * event_ptr;

	event_ptr = lash_event_new();
  if (event_ptr == NULL)
  {
    return NULL;
  }

	lash_event_set_type(event_ptr, type);
	return event_ptr;
}

lash_event_t * lash_event_new_with_all(enum LASH_Event_Type type, const char * string)
{
	lash_event_t * event_ptr;

	event_ptr = lash_event_new_with_type(type);
  if (event_ptr == NULL)
  {
    return NULL;
  }

  if (string != NULL)
  {
    event_ptr->string = strdup(string);
    if (event_ptr->string == NULL)
    {
      log_error("strdup() failed for event string '%s'", string);
      free(event_ptr);
      return NULL;
    }
  }

	return event_ptr;
}

void lash_event_destroy(lash_event_t * event_ptr)
{
  free(event_ptr->string);
  free(event_ptr);
}

enum LASH_Event_Type lash_event_get_type(const lash_event_t * event_ptr)
{
  return event_ptr->type;
}

const char * lash_event_get_string(const lash_event_t * event_ptr)
{
  return event_ptr->string;
}

void lash_event_set_type(lash_event_t * event_ptr, enum LASH_Event_Type type)
{
  event_ptr->type = type;
}

void lash_event_set_string(lash_event_t * event_ptr, const char * string)
{
  char * dup;

  if (string != NULL)
  {
    dup = strdup(string);
    if (dup == NULL)
    {
      log_error("strdup() failed for event string '%s'", string);
      ASSERT_NO_PASS;
      return;
    }
  }
  else
  {
    dup = NULL;
  }

  free(event_ptr->string);
  event_ptr->string = dup;
}

const char * lash_event_get_project(const lash_event_t * event_ptr)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  return NULL;
}

void lash_event_set_project(lash_event_t * event_ptr, const char * project)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}

void lash_event_get_client_id(const lash_event_t * event_ptr, uuid_t id)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}
 
void lash_event_set_client_id(lash_event_t * event_ptr, uuid_t id)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}

unsigned char lash_event_get_alsa_client_id(const lash_event_t * event_ptr)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  return 0;
}

unsigned char lash_str_get_alsa_client_id(const char * str)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  /* this is an undocumented function and probably internal one that sneaked to the public API */
  return 0;
}

void lash_jack_client_name(lash_client_t * client_ptr, const char * name)
{
  /* nothing to do, ladish detects jack client name through jack server */
}

void lash_str_set_alsa_client_id(char * str, unsigned char alsa_id)
{
  /* nothing to do, ladish detects alsa id through alsapid.so, jack and a2jmidid */
  /* this is an undocumented function and probably internal one that sneaked to the public API */
}

void lash_event_set_alsa_client_id(lash_event_t * event_ptr, unsigned char alsa_id)
{
  /* set event type, so we can silently ignore the event, when sent */
	lash_event_set_type(event_ptr, LASH_Alsa_Client_ID);
}

void lash_alsa_client_id(lash_client_t * client, unsigned char id)
{
  /* nothing to do, ladish detects alsa id through alsapid.so, jack and a2jmidid */
}

lash_config_t * lash_config_new(void)
{
  struct _lash_config * config_ptr;

  config_ptr = malloc(sizeof(struct _lash_config));
  if (config_ptr == NULL)
  {
    log_error("malloc() failed to allocate lash event struct");
    return NULL;
  }

  config_ptr->key = NULL;
  config_ptr->value = NULL;
  config_ptr->size = 0;

  return config_ptr;
}

lash_config_t * lash_config_dup(const lash_config_t * src_ptr)
{
	lash_config_t * dst_ptr;

	dst_ptr = lash_config_new();
  if (dst_ptr == NULL)
  {
    return NULL;
  }

  ASSERT(src_ptr->key != NULL);
  dst_ptr->key = strdup(src_ptr->key);
  if (dst_ptr->key == NULL)
  {
    log_error("strdup() failed for config key '%s'", src_ptr->key);
    free(dst_ptr);
    return NULL;
  }

  if (dst_ptr->value != NULL)
  {
    dst_ptr->value = malloc(src_ptr->size);
    if (dst_ptr->value == NULL)
    {
      log_error("strdup() failed for config value with size %zu", src_ptr->size);
      free(dst_ptr->key);
      free(dst_ptr);
      return NULL;
    }

		memcpy(dst_ptr->value, src_ptr->value, src_ptr->size);
    dst_ptr->size = src_ptr->size;
  }

	return dst_ptr;
}

lash_config_t * lash_config_new_with_key(const char * key)
{
	lash_config_t * config_ptr;

	config_ptr = lash_config_new();
  if (config_ptr == NULL)
  {
    return NULL;
  }

  config_ptr->key = strdup(key);
  if (config_ptr->key == NULL)
  {
    log_error("strdup() failed for config key '%s'", key);
    free(config_ptr);
    return NULL;
  }

	return config_ptr;
}

void lash_config_destroy(lash_config_t * config_ptr)
{
  free(config_ptr->key);
  free(config_ptr->value);
  free(config_ptr);
}

const char * lash_config_get_key(const lash_config_t * config_ptr)
{
  return config_ptr->key;
}

const void * lash_config_get_value(const lash_config_t * config_ptr)
{
  return config_ptr->value;
}

size_t lash_config_get_value_size(const lash_config_t * config_ptr)
{
  return config_ptr->size;
}

void lash_config_set_key(lash_config_t * config_ptr, const char * key)
{
  char * dup;

  ASSERT(key != NULL);

  dup = strdup(key);
  if (dup == NULL)
  {
    log_error("strdup() failed for config key '%s'", key);
    ASSERT_NO_PASS;
    return;
  }

  free(config_ptr->key);
  config_ptr->key = dup;
}

void lash_config_set_value(lash_config_t * config_ptr, const void * value, size_t value_size)
{
  void * buf;

  if (value != NULL)
  {
		buf = malloc(value_size);
    if (buf == NULL)
    {
      log_error("malloc() failed for config value with size %zu", value_size);
      ASSERT_NO_PASS;
      return;
    }

		memcpy(buf, value, value_size);
  }
  else
  {
    buf = NULL;
    value_size = 0;
  }

  free(config_ptr->value);
  config_ptr->value = buf;
  config_ptr->size = value_size;
}

uint32_t lash_config_get_value_int(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(uint32_t));
	return ntohl(*(const uint32_t *)lash_config_get_value(config_ptr));
}

float lash_config_get_value_float(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(float));
	return *(const float *)lash_config_get_value(config_ptr);
}

double lash_config_get_value_double(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(double));
	return *(const double *)lash_config_get_value(config_ptr);
}

const char * lash_config_get_value_string(const lash_config_t * config_ptr)
{
  const char * string;
  size_t len;
  void * ptr;

  string = lash_config_get_value(config_ptr);
  len = lash_config_get_value_size(config_ptr);
  ptr = memchr(string, 0, len);
  ASSERT(ptr != NULL);
  return string;
}

void lash_config_set_value_int(lash_config_t * config_ptr, uint32_t value)
{
	value = htonl(value);
	lash_config_set_value(config_ptr, &value, sizeof(uint32_t));
}

void lash_config_set_value_float(lash_config_t * config_ptr, float value)
{
	lash_config_set_value(config_ptr, &value, sizeof(float));
}

void lash_config_set_value_double(lash_config_t * config_ptr, double value)
{
	lash_config_set_value(config_ptr, &value, sizeof(double));
}

void lash_config_set_value_string(lash_config_t * config_ptr, const char * value)
{
	lash_config_set_value(config_ptr, value, strlen(value) + 1);
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
