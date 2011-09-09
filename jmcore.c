/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the JACK multicore (snake)
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

#include <unistd.h>
#include <signal.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "dbus/helpers.h"
#include "dbus/error.h"
#include "dbus_constants.h"

extern const struct dbus_interface_descriptor g_interface;

static const char * g_dbus_unique_name;
static dbus_object_path g_object;
static bool g_quit;
static unsigned int g_unique_index;

struct list_head g_pairs;

struct port_pair
{
  struct list_head siblings;
  jack_client_t * client;
  bool dead;
  bool midi;
  jack_port_t * input_port;
  jack_port_t * output_port;
  char * input_port_name;
  char * output_port_name;
};

#define pair_ptr ((struct port_pair *)arg)

void shutdown_callback(void * arg)
{
  pair_ptr->dead = true;
}

int process_callback(jack_nframes_t nframes, void * arg)
{
  void * input;
  void * output;
  jack_midi_event_t midi_event;
  jack_nframes_t midi_event_index;

  input = jack_port_get_buffer(pair_ptr->input_port, nframes);
  output = jack_port_get_buffer(pair_ptr->output_port, nframes);

  if (!pair_ptr->midi)
  {
    memcpy(output, input, nframes * sizeof(jack_default_audio_sample_t));
  }
  else
  {
    jack_midi_clear_buffer(output);
    midi_event_index = 0;
    while (jack_midi_event_get(&midi_event, input, midi_event_index) == 0)
    {
      jack_midi_event_write(output, midi_event.time, midi_event.buffer, midi_event.size);
      midi_event_index++;
    }
  }

  return 0;
}

#undef pair_ptr

static void destroy_pair(struct port_pair * pair_ptr)
{
  list_del(&pair_ptr->siblings);
  jack_client_close(pair_ptr->client);
  free(pair_ptr->input_port_name);
  free(pair_ptr->output_port_name);
  free(pair_ptr);
}

static void bury_zombie_pairs(void)
{
  struct list_head * node_ptr;
  struct list_head * temp_node_ptr;
  struct port_pair * pair_ptr;

  list_for_each_safe(node_ptr, temp_node_ptr, &g_pairs)
  {
    pair_ptr = list_entry(node_ptr, struct port_pair, siblings);
    if (pair_ptr->dead)
    {
      log_info("Bury zombie '%s':'%s'", pair_ptr->input_port_name, pair_ptr->output_port_name);
      destroy_pair(pair_ptr);
    }
  }
}

static bool connect_dbus(void)
{
  int ret;

  dbus_error_init(&cdbus_g_dbus_error);

  cdbus_g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &cdbus_g_dbus_error);
  if (dbus_error_is_set(&cdbus_g_dbus_error))
  {
    log_error("Failed to get bus: %s", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    goto fail;
  }

  g_dbus_unique_name = dbus_bus_get_unique_name(cdbus_g_dbus_connection);
  if (g_dbus_unique_name == NULL)
  {
    log_error("Failed to read unique bus name");
    goto unref_connection;
  }

  log_info("Connected to local session bus, unique name is \"%s\"", g_dbus_unique_name);

  ret = dbus_bus_request_name(cdbus_g_dbus_connection, JMCORE_SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &cdbus_g_dbus_error);
  if (ret == -1)
  {
    log_error("Failed to acquire bus name: %s", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    goto unref_connection;
  }

  if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
  {
    log_error("Requested connection name already exists");
    goto unref_connection;
  }

  g_object = dbus_object_path_new(JMCORE_OBJECT_PATH, &g_interface, NULL, NULL);
  if (g_object == NULL)
  {
    goto unref_connection;
  }

  if (!dbus_object_path_register(cdbus_g_dbus_connection, g_object))
  {
    goto destroy_control_object;
  }

  return true;

destroy_control_object:
  dbus_object_path_destroy(cdbus_g_dbus_connection, g_object);
unref_connection:
  dbus_connection_unref(cdbus_g_dbus_connection);

fail:
  return false;
}

static void disconnect_dbus(void)
{
  dbus_object_path_destroy(cdbus_g_dbus_connection, g_object);
  dbus_connection_unref(cdbus_g_dbus_connection);
}

void term_signal_handler(int signum)
{
  log_info("Caught signal %d (%s), terminating", signum, strsignal(signum));
  g_quit = true;
}

bool install_term_signal_handler(int signum, bool ignore_if_already_ignored)
{
  sig_t sigh;

  sigh = signal(signum, term_signal_handler);
  if (sigh == SIG_ERR)
  {
    log_error("signal() failed to install handler function for signal %d.", signum);
    return false;
  }

  if (sigh == SIG_IGN && ignore_if_already_ignored)
  {
    signal(SIGTERM, SIG_IGN);
  }

  return true;
}

int main(int argc, char ** argv)
{
  INIT_LIST_HEAD(&g_pairs);

  install_term_signal_handler(SIGTERM, false);
  install_term_signal_handler(SIGINT, true);

  if (!connect_dbus())
  {
    log_error("Failed to connect to D-Bus");
    return 1;
  }

  while (!g_quit)
  {
    dbus_connection_read_write_dispatch(cdbus_g_dbus_connection, 50);
    bury_zombie_pairs();
  }

  while (!list_empty(&g_pairs))
  {
    destroy_pair(list_entry(g_pairs.next, struct port_pair, siblings));
  }

  disconnect_dbus();
  return 0;
}

/***************************************************************************/
/* D-Bus interface implementation */

static void jmcore_get_pid(struct dbus_method_call * call_ptr)
{
  dbus_int64_t pid;

  pid = getpid();

  method_return_new_single(call_ptr, DBUS_TYPE_INT64, &pid);
}

static void jmcore_create(struct dbus_method_call * call_ptr)
{
  dbus_bool_t midi;
  const char * input;
  const char * output;
  struct port_pair * pair_ptr;
  int ret;
  char client_name[256];

  g_unique_index++;
  sprintf(client_name, "jmcore-%u", g_unique_index);

  dbus_error_init(&cdbus_g_dbus_error);
  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_BOOLEAN, &midi,
        DBUS_TYPE_STRING, &input,
        DBUS_TYPE_STRING, &output,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    goto exit;
  }

  pair_ptr = malloc(sizeof(struct port_pair));
  if (pair_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Allocation of port pair structure failed");
    goto exit;
  }

  pair_ptr->input_port_name = strdup(input);
  if (pair_ptr->input_port_name == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Allocation of port name buffer failed");
    goto free_pair;
  }

  pair_ptr->output_port_name = strdup(output);
  if (pair_ptr->input_port_name == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Allocation of port name buffer failed");
    goto free_input_name;
  }

  pair_ptr->client = jack_client_open(client_name, JackNoStartServer, NULL);
  if (pair_ptr->client == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot connect to JACK server");
    goto free_output_name;
  }

  pair_ptr->midi = midi;
  pair_ptr->dead = false;

  ret = jack_set_process_callback(pair_ptr->client, process_callback, pair_ptr);
  if (ret != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK process callback setup failed");
    goto close_client;
  }

  jack_on_shutdown(pair_ptr->client, shutdown_callback, pair_ptr);

  pair_ptr->input_port = jack_port_register(pair_ptr->client, input, midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (pair_ptr->input_port == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Port '%s' registration failed.", input);
    goto free_output_name;
  }

  pair_ptr->output_port = jack_port_register(pair_ptr->client, output, midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (pair_ptr->input_port == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Port '%s' registration failed.", output);
    goto unregister_input_port;
  }

  list_add_tail(&pair_ptr->siblings, &g_pairs);

  ret = jack_activate(pair_ptr->client);
  if (ret != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK client activation failed");
    goto remove_from_list;
  }

  method_return_new_void(call_ptr);
  goto exit;

remove_from_list:
  list_del(&pair_ptr->siblings);
//unregister_output_port:
  jack_port_unregister(pair_ptr->client, pair_ptr->output_port);
unregister_input_port:
  jack_port_unregister(pair_ptr->client, pair_ptr->input_port);
close_client:
  jack_client_close(pair_ptr->client);
free_output_name:
  free(pair_ptr->output_port_name);
free_input_name:
  free(pair_ptr->input_port_name);
free_pair:
  free(pair_ptr);
exit:
  return;
}

static void jmcore_destroy(struct dbus_method_call * call_ptr)
{
  const char * port;
  struct list_head * node_ptr;
  struct port_pair * pair_ptr;

  dbus_error_init(&cdbus_g_dbus_error);
  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &port, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  list_for_each(node_ptr, &g_pairs)
  {
    pair_ptr = list_entry(node_ptr, struct port_pair, siblings);
    if (strcmp(pair_ptr->input_port_name, port) == 0 ||
        strcmp(pair_ptr->output_port_name, port) == 0)
    {
      destroy_pair(pair_ptr);
      method_return_new_void(call_ptr);
      return;
    }
  }

  lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "port '%s' not found.", port);
  return;
}

static void jmcore_exit(struct dbus_method_call * call_ptr)
{
  log_info("Exit command received through D-Bus");
  g_quit = true;
  method_return_new_void(call_ptr);
}

METHOD_ARGS_BEGIN(get_pid, "Get process ID")
  METHOD_ARG_DESCRIBE_OUT("process_id", DBUS_TYPE_INT64_AS_STRING, "Process ID")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(create, "Create port pair")
  METHOD_ARG_DESCRIBE_IN("midi", "b", "Whether to create midi or audio ports")
  METHOD_ARG_DESCRIBE_IN("input_port", "s", "Input port name")
  METHOD_ARG_DESCRIBE_IN("output_port", "s", "Output port name")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(destroy, "Destroy port pair")
  METHOD_ARG_DESCRIBE_IN("port", "s", "Port name")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(exit, "Tell jmcore D-Bus service to exit")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(get_pid, jmcore_get_pid)
  METHOD_DESCRIBE(create, jmcore_create)
  METHOD_DESCRIBE(destroy, jmcore_destroy)
  METHOD_DESCRIBE(exit, jmcore_exit)
METHODS_END

INTERFACE_BEGIN(g_interface, JMCORE_IFACE)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
