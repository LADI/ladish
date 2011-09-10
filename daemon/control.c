/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains code of the D-Bus control interface helpers
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

#include "common.h"
#include "studio.h"
#include "../dbus/error.h"
#include "control.h"
#include "../dbus_constants.h"
#include "cmd.h"
#include "room.h"
#include "../lib/wkports.h"
#include "../proxies/conf_proxy.h"
#include "conf.h"

#define INTERFACE_NAME IFACE_CONTROL

/* 805e485f-65e4-4c37-a959-2a3b60b3c270 */
UUID_DEFINE(empty_room,0x80,0x5E,0x48,0x5F,0x65,0xE4,0x4C,0x37,0xA9,0x59,0x2A,0x3B,0x60,0xB3,0xC2,0x70);

/* c603f2a0-d96a-493e-a8cf-55581d950aa9 */
UUID_DEFINE(basic_room,0xC6,0x03,0xF2,0xA0,0xD9,0x6A,0x49,0x3E,0xA8,0xCF,0x55,0x58,0x1D,0x95,0x0A,0xA9);

/* f3c6070d-e60f-4449-8b62-8c232df3eb2c */
/* room with 4 audio capture ports, 4 audio playback ports, 2 midi capture ports and 2 midi playback ports */
UUID_DEFINE(router_4x4x2x2_room,0xF3,0xC6,0x07,0x0D,0xE6,0x0F,0x44,0x49,0x8B,0x62,0x8C,0x23,0x2D,0xF3,0xEB,0x2C);

/* a791a5f7-a6a2-44d5-9113-9115960300a4 */
/* room with 10 audio capture ports, 10 audio playback ports, 4 midi capture ports and 4 midi playback ports */
UUID_DEFINE(router_10x10x4x4_room,0xA7,0x91,0xA5,0xF7,0xA6,0xA2,0x44,0xD5,0x91,0x13,0x91,0x15,0x96,0x03,0x00,0xA4);

/* 42d580f4-c5e5-4909-bc65-dfd96b3cbfce */
/* room with 40 audio capture ports, 40 audio playback ports, 16 midi capture ports and 16 midi playback ports */
UUID_DEFINE(router_40x40x16x16_room,0x42,0xD5,0x80,0xF4,0xC5,0xE5,0x49,0x09,0xBC,0x65,0xDF,0xD9,0x6B,0x3C,0xBF,0xCE);

const unsigned char * ladish_wkport_router_audio_capture[] =
{
  ladish_wkport_router_audio_capture_1,
  ladish_wkport_router_audio_capture_2,
  ladish_wkport_router_audio_capture_3,
  ladish_wkport_router_audio_capture_4,
  ladish_wkport_router_audio_capture_5,
  ladish_wkport_router_audio_capture_6,
  ladish_wkport_router_audio_capture_7,
  ladish_wkport_router_audio_capture_8,
  ladish_wkport_router_audio_capture_9,
  ladish_wkport_router_audio_capture_10,
  ladish_wkport_router_audio_capture_11,
  ladish_wkport_router_audio_capture_12,
  ladish_wkport_router_audio_capture_13,
  ladish_wkport_router_audio_capture_14,
  ladish_wkport_router_audio_capture_15,
  ladish_wkport_router_audio_capture_16,
  ladish_wkport_router_audio_capture_17,
  ladish_wkport_router_audio_capture_18,
  ladish_wkport_router_audio_capture_19,
  ladish_wkport_router_audio_capture_20,
  ladish_wkport_router_audio_capture_21,
  ladish_wkport_router_audio_capture_22,
  ladish_wkport_router_audio_capture_23,
  ladish_wkport_router_audio_capture_24,
  ladish_wkport_router_audio_capture_25,
  ladish_wkport_router_audio_capture_26,
  ladish_wkport_router_audio_capture_27,
  ladish_wkport_router_audio_capture_28,
  ladish_wkport_router_audio_capture_29,
  ladish_wkport_router_audio_capture_30,
  ladish_wkport_router_audio_capture_31,
  ladish_wkport_router_audio_capture_32,
  ladish_wkport_router_audio_capture_33,
  ladish_wkport_router_audio_capture_34,
  ladish_wkport_router_audio_capture_35,
  ladish_wkport_router_audio_capture_36,
  ladish_wkport_router_audio_capture_37,
  ladish_wkport_router_audio_capture_38,
  ladish_wkport_router_audio_capture_39,
  ladish_wkport_router_audio_capture_40,
};

const unsigned char * ladish_wkport_router_audio_playback[] =
{
  ladish_wkport_router_audio_playback_1,
  ladish_wkport_router_audio_playback_2,
  ladish_wkport_router_audio_playback_3,
  ladish_wkport_router_audio_playback_4,
  ladish_wkport_router_audio_playback_5,
  ladish_wkport_router_audio_playback_6,
  ladish_wkport_router_audio_playback_7,
  ladish_wkport_router_audio_playback_8,
  ladish_wkport_router_audio_playback_9,
  ladish_wkport_router_audio_playback_10,
  ladish_wkport_router_audio_playback_11,
  ladish_wkport_router_audio_playback_12,
  ladish_wkport_router_audio_playback_13,
  ladish_wkport_router_audio_playback_14,
  ladish_wkport_router_audio_playback_15,
  ladish_wkport_router_audio_playback_16,
  ladish_wkport_router_audio_playback_17,
  ladish_wkport_router_audio_playback_18,
  ladish_wkport_router_audio_playback_19,
  ladish_wkport_router_audio_playback_20,
  ladish_wkport_router_audio_playback_21,
  ladish_wkport_router_audio_playback_22,
  ladish_wkport_router_audio_playback_23,
  ladish_wkport_router_audio_playback_24,
  ladish_wkport_router_audio_playback_25,
  ladish_wkport_router_audio_playback_26,
  ladish_wkport_router_audio_playback_27,
  ladish_wkport_router_audio_playback_28,
  ladish_wkport_router_audio_playback_29,
  ladish_wkport_router_audio_playback_30,
  ladish_wkport_router_audio_playback_31,
  ladish_wkport_router_audio_playback_32,
  ladish_wkport_router_audio_playback_33,
  ladish_wkport_router_audio_playback_34,
  ladish_wkport_router_audio_playback_35,
  ladish_wkport_router_audio_playback_36,
  ladish_wkport_router_audio_playback_37,
  ladish_wkport_router_audio_playback_38,
  ladish_wkport_router_audio_playback_39,
  ladish_wkport_router_audio_playback_40,
};

const unsigned char * ladish_wkport_router_midi_capture[] =
{
  ladish_wkport_router_midi_capture_1,
  ladish_wkport_router_midi_capture_2,
  ladish_wkport_router_midi_capture_3,
  ladish_wkport_router_midi_capture_4,
  ladish_wkport_router_midi_capture_5,
  ladish_wkport_router_midi_capture_6,
  ladish_wkport_router_midi_capture_7,
  ladish_wkport_router_midi_capture_8,
  ladish_wkport_router_midi_capture_9,
  ladish_wkport_router_midi_capture_10,
  ladish_wkport_router_midi_capture_11,
  ladish_wkport_router_midi_capture_12,
  ladish_wkport_router_midi_capture_13,
  ladish_wkport_router_midi_capture_14,
  ladish_wkport_router_midi_capture_15,
  ladish_wkport_router_midi_capture_16,
};

const unsigned char * ladish_wkport_router_midi_playback[] =
{
  ladish_wkport_router_midi_playback_1,
  ladish_wkport_router_midi_playback_2,
  ladish_wkport_router_midi_playback_3,
  ladish_wkport_router_midi_playback_4,
  ladish_wkport_router_midi_playback_5,
  ladish_wkport_router_midi_playback_6,
  ladish_wkport_router_midi_playback_7,
  ladish_wkport_router_midi_playback_8,
  ladish_wkport_router_midi_playback_9,
  ladish_wkport_router_midi_playback_10,
  ladish_wkport_router_midi_playback_11,
  ladish_wkport_router_midi_playback_12,
  ladish_wkport_router_midi_playback_13,
  ladish_wkport_router_midi_playback_14,
  ladish_wkport_router_midi_playback_15,
  ladish_wkport_router_midi_playback_16,
};

static struct list_head g_room_templates;

static bool create_empty_room_template(const uuid_t uuid_ptr, const char * name, ladish_room_handle * room_ptr)
{
  if (!ladish_room_create_template(uuid_ptr, name, room_ptr))
  {
    log_error("ladish_room_create_template() failed for room template \"%s\".", name);
    return false;
  }

  return true;
}

struct room_descriptor
{
  const char * name;
  ladish_room_handle room;
  ladish_graph_handle graph;
  ladish_client_handle capture;
  ladish_client_handle playback;
};

static
bool
create_room_template(
  const uuid_t uuid_ptr,
  const char * name,
  struct room_descriptor * room_descriptor_ptr)
{
  ladish_room_handle room;
  ladish_graph_handle graph;
  ladish_client_handle capture;
  ladish_client_handle playback;

  if (!create_empty_room_template(uuid_ptr, name, &room))
  {
    log_error("ladish_room_create() failed for room template \"%s\".", name);
    goto fail;
  }

  graph = ladish_room_get_graph(room);

  if (!ladish_client_create(ladish_wkclient_capture, &capture))
  {
    log_error("ladish_client_create() failed to create capture client to room template \"%s\".", name);
    goto fail_destroy;
  }

  if (!ladish_graph_add_client(graph, capture, "Capture", false))
  {
    log_error("ladish_graph_add_client() failed to add capture client to room template \"%s\".", name);
    goto fail_destroy;
  }

  if (!ladish_client_create(ladish_wkclient_playback, &playback))
  {
    log_error("ladish_client_create() failed to create playback client to room template \"%s\".", name);
    goto fail_destroy;
  }

  if (!ladish_graph_add_client(graph, playback, "Playback", false))
  {
    log_error("ladish_graph_add_client() failed to add playback client to room template \"%s\".", name);
    goto fail_destroy;
  }

  room_descriptor_ptr->name = ladish_room_get_name(room);
  room_descriptor_ptr->room = room;
  room_descriptor_ptr->graph = graph;
  room_descriptor_ptr->capture = capture;
  room_descriptor_ptr->playback = playback;

  return true;

fail_destroy:
  ladish_room_destroy(room);    /* this will destroy the graph clients as well */
fail:
  return false;
}

static
bool
create_room_template_port(
  struct room_descriptor * room_descriptor_ptr,
  const uuid_t uuid_ptr,
  const char * name,
  uint32_t type,
  uint32_t flags)
{
  ladish_port_handle port;
  bool playback;
  ladish_client_handle client;

  playback = JACKDBUS_PORT_IS_INPUT(flags);
  ASSERT(playback || JACKDBUS_PORT_IS_OUTPUT(flags)); /* playback or capture */
  ASSERT(!(playback && JACKDBUS_PORT_IS_OUTPUT(flags))); /* but not both */
  client = playback ? room_descriptor_ptr->playback : room_descriptor_ptr->capture;

  if (!ladish_port_create(uuid_ptr, true, &port))
  {
    log_error("Creation of room template \"%s\" %s port \"%s\" failed.", room_descriptor_ptr->name, playback ? "playback" : "capture", name);
    return false;
  }

  if (!ladish_graph_add_port(room_descriptor_ptr->graph, client, port, name, type, flags, false))
  {
    log_error("Adding of room template \"%s\" %s port \"%s\" to graph failed.", room_descriptor_ptr->name, playback ? "playback" : "capture", name);
    ladish_port_destroy(port);
    return false;
  }

  return true;
}

#define ADD_ROUTER_PORT(index, prefix, uuid_ptr, protocol, flags)       \
  sprintf(port_name_buffer, prefix "%u", index + 1);                    \
  if (!create_room_template_port(                                       \
        &room_descriptor,                                               \
        uuid_ptr,                                                       \
        port_name_buffer,                                               \
        protocol,                                                       \
        flags))                                                         \
  {                                                                     \
    goto fail;                                                          \
  }

void
create_router_room_template(
  const uuid_t uuid_ptr,
  const char * name,
  unsigned int audio_capture,
  unsigned int audio_playback,
  unsigned int midi_capture,
  unsigned int midi_playback)
{
  struct room_descriptor room_descriptor;
  char port_name_buffer[100];
  unsigned int i;

  if (create_room_template(uuid_ptr, name, &room_descriptor))
  {
    for (i = 0; i < audio_capture; i++)
    {
      ADD_ROUTER_PORT(i, "audio_capture_", ladish_wkport_router_audio_capture[i], JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_OUTPUT);
    }

    for (i = 0; i < audio_playback; i++)
    {
      ADD_ROUTER_PORT(i, "audio_playback_", ladish_wkport_router_audio_playback[i], JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_INPUT);
    }

    for (i = 0; i < midi_capture; i++)
    {
      ADD_ROUTER_PORT(i, "midi_capture_", ladish_wkport_router_midi_capture[i], JACKDBUS_PORT_TYPE_MIDI, JACKDBUS_PORT_FLAG_OUTPUT);
    }

    for (i = 0; i < midi_playback; i++)
    {
      ADD_ROUTER_PORT(i, "midi_playback_", ladish_wkport_router_midi_playback[i], JACKDBUS_PORT_TYPE_MIDI, JACKDBUS_PORT_FLAG_INPUT);
    }

    list_add_tail(ladish_room_get_list_node(room_descriptor.room), &g_room_templates);
  }

  return;

fail:
  ladish_room_destroy(room_descriptor.room); /* this will destroy the graph clients and ports as well */
}

#undef ADD_ROUTER_PORT

void create_builtin_room_templates(void)
{
  struct room_descriptor room_descriptor;

#if 0 /* the empty template is useless until there is a functionality to add new room ports */
  if (create_empty_room_template(empty_room, "Empty", &room_descriptor.room))
  {
    list_add_tail(ladish_room_get_list_node(room_descriptor.room), &g_room_templates);
  }
#endif

  if (create_room_template(basic_room, "Basic", &room_descriptor))
  {
    if (!create_room_template_port(&room_descriptor, ladish_wkport_capture_left, "Left", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_OUTPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_capture_right, "Right", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_OUTPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_midi_capture, "MIDI", JACKDBUS_PORT_TYPE_MIDI, JACKDBUS_PORT_FLAG_OUTPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_playback_left, "Left", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_INPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_playback_right, "Right", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_INPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_monitor_left, "Monitor Left", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_INPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_monitor_right, "Monitor Right", JACKDBUS_PORT_TYPE_AUDIO, JACKDBUS_PORT_FLAG_INPUT))
    {
      goto fail;
    }

    if (!create_room_template_port(&room_descriptor, ladish_wkport_midi_playback, "MIDI", JACKDBUS_PORT_TYPE_MIDI, JACKDBUS_PORT_FLAG_INPUT))
    {
      goto fail;
    }

    list_add_tail(ladish_room_get_list_node(room_descriptor.room), &g_room_templates);
  }

  create_router_room_template(router_4x4x2x2_room, "Router 4x4x2x2", 4, 4, 2, 2);
  create_router_room_template(router_10x10x4x4_room, "Router 10x10x4x4", 10, 10, 4, 4);
  create_router_room_template(router_40x40x16x16_room, "Router 40x40x16x16", 40, 40, 16, 16);

  return;

fail:
  ladish_room_destroy(room_descriptor.room); /* this will destroy the graph clients and ports as well */
}

void create_room_templates(void)
{
  create_builtin_room_templates();
}

void maybe_create_room_templates(void)
{
  if (list_empty(&g_room_templates))
  {
    create_room_templates();
  }
}

bool room_templates_init(void)
{
  INIT_LIST_HEAD(&g_room_templates);

  return true;
}

void room_templates_uninit(void)
{
  struct list_head * node_ptr;
  ladish_room_handle room;

  while (!list_empty(&g_room_templates))
  {
    node_ptr = g_room_templates.next;
    list_del(node_ptr);
    room = ladish_room_from_list_node(node_ptr);
    ladish_room_destroy(room);
  }
}

bool room_templates_enum(void * context, bool (* callback)(void * context, ladish_room_handle room))
{
  struct list_head * node_ptr;

  maybe_create_room_templates();

  list_for_each(node_ptr, &g_room_templates)
  {
    if (!callback(context, ladish_room_from_list_node(node_ptr)))
    {
      return false;
    }
  }

  return true;
}

ladish_room_handle find_room_template_by_name(const char * name)
{
  ladish_room_handle room;
  struct list_head * node_ptr;

  maybe_create_room_templates();

  list_for_each(node_ptr, &g_room_templates)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (strcmp(ladish_room_get_name(room), name) == 0)
    {
      return room;
    }
  }

  return NULL;
}

ladish_room_handle find_room_template_by_uuid(const uuid_t uuid_ptr)
{
  ladish_room_handle room;
  struct list_head * node_ptr;
  uuid_t uuid;

  maybe_create_room_templates();

  list_for_each(node_ptr, &g_room_templates)
  {
    room = ladish_room_from_list_node(node_ptr);
    ladish_room_get_uuid(room, uuid);
    if (uuid_compare(uuid, uuid_ptr) == 0)
    {
      return room;
    }
  }

  return NULL;
}

static void ladish_is_studio_loaded(struct cdbus_method_call * call_ptr)
{
  DBusMessageIter iter;
  dbus_bool_t is_loaded;

  is_loaded = ladish_studio_is_loaded();

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &is_loaded))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

#define array_iter_ptr ((DBusMessageIter *)context)

static bool get_studio_list_callback(void * call_ptr, void * context, const char * studio, uint32_t modtime)
{
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  bool ret;

  ret = false;

  if (!dbus_message_iter_open_container(array_iter_ptr, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    goto exit;

  if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &studio))
    goto close_struct;

  if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
    goto close_struct;

/*   if (!maybe_add_dict_entry_string(&dict_iter, "Description", xxx)) */
/*     goto close_dict; */

  if (!cdbus_add_dict_entry_uint32(&dict_iter, "Modification Time", modtime))
    goto close_dict;

  ret = true;

close_dict:
  if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
    ret = false;

close_struct:
  if (!dbus_message_iter_close_container(array_iter_ptr, &struct_iter))
    ret = false;

exit:
  return ret;
}

static void ladish_get_studio_list(struct cdbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
  {
    goto fail_unref;
  }

  if (!ladish_studios_iterate(call_ptr, &array_iter, get_studio_list_callback))
  {
    dbus_message_iter_close_container(&iter, &array_iter);
    if (call_ptr->reply == NULL)
      goto fail_unref;

    /* studios_iterate or get_studio_list_callback() composed error reply */
    return;
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

static void ladish_load_studio(struct cdbus_method_call * call_ptr)
{
  const char * name;
  bool autostart;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("Load studio request (%s)", name);

  if (!conf_get_bool(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, &autostart))
  {
    autostart = LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT;
  }

  if (autostart)
  {
    log_info("Studio will be autostarted upon load");
  }

  if (ladish_command_load_studio(call_ptr, ladish_studio_get_cmd_queue(), name, autostart))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_delete_studio(struct cdbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_studio_delete(call_ptr, name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_new_studio(struct cdbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("New studio request (%s)", name);

  if (ladish_command_new_studio(call_ptr, ladish_studio_get_cmd_queue(), name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_get_application_list(struct cdbus_method_call * call_ptr)
{
  DBusMessageIter iter;
  DBusMessageIter array_iter;
#if 0
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  struct list_head * node_ptr;
  struct lash_appdb_entry * entry_ptr;
#endif

  log_info("Getting applications list");

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
  {
    goto fail_unref;
  }

#if 0
  list_for_each(node_ptr, &g_server->appdb)
  {
    entry_ptr = list_entry(node_ptr, struct lash_appdb_entry, siblings);

    if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
      goto fail_unref;

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, (const void *) &entry_ptr->name))
    {
      dbus_message_iter_close_container(&iter, &array_iter);
      goto fail_unref;
    }

    if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "GenericName", entry_ptr->generic_name))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "Comment", entry_ptr->comment))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "Icon", entry_ptr->icon))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
      goto fail_unref;
  }
#endif

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;
fail:
  return;
}

#define array_iter_ptr ((DBusMessageIter *)context)

bool room_template_list_filler(void * context, ladish_room_handle room)
{
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  const char * name;

  name = ladish_room_get_name(room);

  if (!dbus_message_iter_open_container(array_iter_ptr, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    return false;

  if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &name))
    return false;

  if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
    return false;

  if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
    return false;

  if (!dbus_message_iter_close_container(array_iter_ptr, &struct_iter))
    return false;

  return true;
}

#undef array_iter_ptr

static void ladish_get_room_template_list(struct cdbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
  {
    goto fail_unref;
  }

  if (!room_templates_enum(&array_iter, room_template_list_filler))
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

static void ladish_delete_room_template(struct cdbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("Delete room request (%s)", name);

  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_create_room_template(struct cdbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("New room request (%s)", name);

  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_exit(struct cdbus_method_call * call_ptr)
{
  log_info("Exit command received through D-Bus");

  if (!ladish_command_exit(NULL, ladish_studio_get_cmd_queue()))
  { /* if queuing of command failed, force exit anyway,
       JACK server will be left started,
       but refusing exit command is worse */
    g_quit = true;
  }

  cdbus_method_return_new_void(call_ptr);
}

void emit_studio_appeared(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "StudioAppeared", "");
}

void emit_studio_disappeared(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "StudioDisappeared", "");
}

void emit_queue_execution_halted(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "QueueExecutionHalted", "");
}

void emit_clean_exit(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "CleanExit", "");
}

METHOD_ARGS_BEGIN(IsStudioLoaded, "Check whether studio D-Bus object is present")
  METHOD_ARG_DESCRIBE_OUT("present", "b", "Whether studio D-Bus object is present")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetStudioList, "Get list of studios")
  METHOD_ARG_DESCRIBE_OUT("studio_list", "a(sa{sv})", "List of studios, name and properties")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(NewStudio, "New studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio, if empty name will be generated")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadStudio, "Load studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio to load")
  METHOD_ARG_DESCRIBE_IN("options", "a{sv}", "Load options")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DeleteStudio, "Delete studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio to delete")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetApplicationList, "Get list of applications that can be launched")
  METHOD_ARG_DESCRIBE_OUT("applications", "a(sa{sv})", "List of applications, name and properties")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetRoomTemplateList, "Get list of room templates")
  METHOD_ARG_DESCRIBE_OUT("room_template_list", "a(sa{sv})", "List of room templates (name and properties)")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(CreateRoomTemplate, "New room template")
  METHOD_ARG_DESCRIBE_IN("room_template name", "s", "Name of the room template")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DeleteRoomTemplate, "Delete room template")
  METHOD_ARG_DESCRIBE_IN("room_template_name", "s", "Name of room template to delete")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Exit, "Tell ladish D-Bus service to exit")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(IsStudioLoaded, ladish_is_studio_loaded)
  METHOD_DESCRIBE(GetStudioList, ladish_get_studio_list)
  METHOD_DESCRIBE(NewStudio, ladish_new_studio)
  METHOD_DESCRIBE(LoadStudio, ladish_load_studio)
  METHOD_DESCRIBE(DeleteStudio, ladish_delete_studio)
  METHOD_DESCRIBE(GetApplicationList, ladish_get_application_list)
  METHOD_DESCRIBE(GetRoomTemplateList, ladish_get_room_template_list)
  METHOD_DESCRIBE(CreateRoomTemplate, ladish_create_room_template)
  METHOD_DESCRIBE(DeleteRoomTemplate, ladish_delete_room_template)
  METHOD_DESCRIBE(Exit, ladish_exit)
METHODS_END

SIGNAL_ARGS_BEGIN(StudioAppeared, "Studio D-Bus object appeared")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioDisappeared, "Studio D-Bus object disappeared")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(QueueExecutionHalted, "Queue execution is halted because of error")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(CleanExit, "Exit was requested")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(StudioAppeared)
  SIGNAL_DESCRIBE(StudioDisappeared)
  SIGNAL_DESCRIBE(QueueExecutionHalted)
  SIGNAL_DESCRIBE(CleanExit)
SIGNALS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_lashd_interface_control, INTERFACE_NAME)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
