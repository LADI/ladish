/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 **************************************************************************
 * This file contains the liblash interface
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

#ifndef __LASH_H__
#define __LASH_H__

#include <uuid/uuid.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define LASH_PROTOCOL_MAJOR 2
#define LASH_PROTOCOL_MINOR 0
#define LASH_PROTOCOL_MAJOR_MASK 0xFFFF0000
#define LASH_PROTOCOL_MINOR_MASK 0x0000FFFF
#define LASH_PROTOCOL(major, minor) ((lash_protocol_t)( ((major << 16) & LASH_PROTOCOL_MAJOR_MASK) | (minor & LASH_PROTOCOL_MINOR_MASK)))
#define LASH_PROTOCOL_VERSION (LASH_PROTOCOL (LASH_PROTOCOL_MAJOR, LASH_PROTOCOL_MINOR))
#define LASH_PROTOCOL_GET_MAJOR(protocol) ((lash_protocol_t) (((protocol & LASH_PROTOCOL_MAJOR_MASK) >> 16)))
#define LASH_PROTOCOL_GET_MINOR(protocol) ((lash_protocol_t) (protocol & LASH_PROTOCOL_MINOR_MASK))
#define LASH_PROTOCOL_IS_VALID(proto) ((LASH_PROTOCOL_GET_MAJOR (proto) == LASH_PROTOCOL_MAJOR) && (LASH_PROTOCOL_GET_MINOR (proto) <= LASH_PROTOCOL_MINOR))
#define LASH_DEFAULT_PORT 14541
#define lash_enabled(client) 0
#define LASH_PORT "14541"

typedef uint32_t lash_protocol_t;

enum LASH_Client_Flag
{
  LASH_Config_Data_Set  = 0x00000001,
  LASH_Config_File      = 0x00000002,
  LASH_Server_Interface = 0x00000004,
  LASH_No_Autoresume    = 0x00000008,
  LASH_Terminal         = 0x00000010,
  LASH_No_Start_Server  = 0x00000020
};

enum LASH_Event_Type
{
  LASH_Client_Name = 1,
  LASH_Jack_Client_Name,
  LASH_Alsa_Client_ID,
  LASH_Save_File,
  LASH_Restore_File,
  LASH_Save_Data_Set,
  LASH_Restore_Data_Set,
  LASH_Save,
  LASH_Quit,
  LASH_Server_Lost,
  LASH_Project_Add,
  LASH_Project_Remove,
  LASH_Project_Dir,
  LASH_Project_Name,
  LASH_Client_Add,
  LASH_Client_Remove,
  LASH_Percentage
};

typedef struct _lash_args lash_args_t;
typedef struct _lash_client lash_client_t;
typedef struct _lash_event lash_event_t;
typedef struct _lash_config lash_config_t;

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

const char * lash_protocol_string(lash_protocol_t protocol);
lash_event_t * lash_event_new(void);
lash_event_t * lash_event_new_with_type(enum LASH_Event_Type type);
lash_event_t * lash_event_new_with_all(enum LASH_Event_Type type, const char * string);
void lash_event_destroy(lash_event_t * event);
enum LASH_Event_Type lash_event_get_type(const lash_event_t * event);
const char * lash_event_get_string(const lash_event_t * event);
const char * lash_event_get_project(const lash_event_t * event);
void lash_event_get_client_id(const lash_event_t * event, uuid_t id);
void lash_event_set_type(lash_event_t * event, enum LASH_Event_Type type);
void lash_event_set_string(lash_event_t * event, const char * string);
void lash_event_set_project(lash_event_t * event, const char * project);
void lash_event_set_client_id(lash_event_t * event, uuid_t id);
void lash_event_set_alsa_client_id(lash_event_t * event, unsigned char alsa_id);
unsigned char lash_event_get_alsa_client_id(const lash_event_t * event);
void lash_str_set_alsa_client_id(char * str, unsigned char alsa_id);
unsigned char lash_str_get_alsa_client_id(const char * str);
lash_args_t * lash_extract_args(int * argc, char *** argv);
void lash_args_destroy(lash_args_t * args);
lash_client_t * lash_init(const lash_args_t * args, const char * client_class, int client_flags, lash_protocol_t protocol);
const char * lash_get_server_name(lash_client_t * client);
unsigned int lash_get_pending_event_count(lash_client_t * client);
lash_event_t * lash_get_event(lash_client_t * client);
unsigned int lash_get_pending_config_count(lash_client_t * client);
lash_config_t * lash_get_config(lash_client_t * client);
void lash_send_event(lash_client_t * client, lash_event_t * event);
void lash_send_config(lash_client_t * client, lash_config_t * config);
int lash_server_connected(lash_client_t * client);
void lash_jack_client_name(lash_client_t * client, const char * name);
void lash_alsa_client_id(lash_client_t * client, unsigned char id);
lash_event_t * lash_event_new(void);
lash_event_t * lash_event_new_with_type(enum LASH_Event_Type type);
lash_event_t * lash_event_new_with_all(enum LASH_Event_Type type, const char * string);
void lash_event_destroy(lash_event_t * event);
enum LASH_Event_Type lash_event_get_type(const lash_event_t * event);
const char * lash_event_get_string(const lash_event_t * event);
const char * lash_event_get_project(const lash_event_t * event);
void lash_event_get_client_id(const lash_event_t * event, uuid_t id);
void lash_event_set_type(lash_event_t * event, enum LASH_Event_Type type);
void lash_event_set_string(lash_event_t * event, const char * string);
void lash_event_set_project(lash_event_t * event, const char * project);
void lash_event_set_client_id(lash_event_t * event, uuid_t id);
void lash_event_set_alsa_client_id(lash_event_t * event, unsigned char alsa_id);
unsigned char lash_event_get_alsa_client_id(const lash_event_t * event);
void lash_str_set_alsa_client_id(char * str, unsigned char alsa_id);
unsigned char lash_str_get_alsa_client_id(const char * str);
lash_config_t * lash_config_new(void);
lash_config_t * lash_config_dup(const lash_config_t * config);
lash_config_t * lash_config_new_with_key(const char * key);
void lash_config_destroy(lash_config_t * config);
const char * lash_config_get_key(const lash_config_t * config);
const void * lash_config_get_value(const lash_config_t * config);
size_t lash_config_get_value_size(const lash_config_t * config);
void lash_config_set_key(lash_config_t * config, const char * key);
void lash_config_set_value(lash_config_t * config, const void * value, size_t value_size);
uint32_t lash_config_get_value_int(const lash_config_t * config);
float lash_config_get_value_float(const lash_config_t * config);
double lash_config_get_value_double(const lash_config_t * config);
const char * lash_config_get_value_string(const lash_config_t * config);
void lash_config_set_value_int(lash_config_t * config, uint32_t value);
void lash_config_set_value_float(lash_config_t * config, float value);
void lash_config_set_value_double(lash_config_t * config, double value);
void lash_config_set_value_string(lash_config_t * config, const char * value);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_H__ */
