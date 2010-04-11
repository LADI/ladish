/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
static jack_client_t * g_jack_client;
static bool g_jack_client_died;

pthread_mutex_t g_mutex;
struct list_head g_new_pairs;
struct list_head g_active_pairs;

struct port_pair
{
  struct list_head siblings;
  bool active;
  bool midi;
  jack_port_t * input_port;
  jack_port_t * output_port;
  char * input_port_name;
  char * output_port_name;
};

void shutdown_callback(void * arg)
{
  g_jack_client_died = true;
}

int process_callback(jack_nframes_t nframes, void * arg)
{
  struct list_head * node_ptr;
  struct list_head * temp_node_ptr;
  struct port_pair * pair_ptr;
  bool locked;
  void * input;
  void * output;
  jack_midi_event_t midi_event;
  jack_nframes_t midi_event_index;

  locked = pthread_mutex_trylock(&g_mutex) == 0;

  if (locked)
  {
    while (!list_empty(&g_new_pairs))
    {
      node_ptr = g_new_pairs.next;
      list_del(node_ptr);
      list_add_tail(node_ptr, &g_active_pairs);
      pair_ptr = list_entry(node_ptr, struct port_pair, siblings);
      pair_ptr->active = true;
    }
  }

  list_for_each_safe(node_ptr, temp_node_ptr, &g_active_pairs)
  {
    pair_ptr = list_entry(node_ptr, struct port_pair, siblings);
    if (locked && !pair_ptr->active)
    {
      list_del_init(node_ptr);
      continue;
    }

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
  }

  if (locked)
  {
    pthread_mutex_unlock(&g_mutex);
  }

  return 0;
}

static void free_pair(struct port_pair * pair_ptr)
{
  if (g_jack_client != NULL)
  {
    jack_port_unregister(g_jack_client, pair_ptr->input_port);
    jack_port_unregister(g_jack_client, pair_ptr->output_port);
  }

  free(pair_ptr->input_port_name);
  free(pair_ptr->output_port_name);
  free(pair_ptr);
}

static void eat_pair_list(struct list_head * list)
{
  struct list_head * node_ptr;

  while (!list_empty(list))
  {
    node_ptr = list->next;
    list_del(node_ptr);
    free_pair(list_entry(node_ptr, struct port_pair, siblings));
  }
}

static bool connect_dbus(void)
{
  int ret;

  dbus_error_init(&g_dbus_error);

  g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &g_dbus_error);
  if (dbus_error_is_set(&g_dbus_error))
  {
    log_error("Failed to get bus: %s", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    goto fail;
  }

  g_dbus_unique_name = dbus_bus_get_unique_name(g_dbus_connection);
  if (g_dbus_unique_name == NULL)
  {
    log_error("Failed to read unique bus name");
    goto unref_connection;
  }

  log_info("Connected to local session bus, unique name is \"%s\"", g_dbus_unique_name);

  ret = dbus_bus_request_name(g_dbus_connection, JMCORE_SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &g_dbus_error);
  if (ret == -1)
  {
    log_error("Failed to acquire bus name: %s", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
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

  if (!dbus_object_path_register(g_dbus_connection, g_object))
  {
    goto destroy_control_object;
  }

  return true;

destroy_control_object:
  dbus_object_path_destroy(g_dbus_connection, g_object);
unref_connection:
  dbus_connection_unref(g_dbus_connection);

fail:
  return false;
}

static void disconnect_dbus(void)
{
  dbus_object_path_destroy(g_dbus_connection, g_object);
  dbus_connection_unref(g_dbus_connection);
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
  int ret;

  INIT_LIST_HEAD(&g_new_pairs);
  INIT_LIST_HEAD(&g_active_pairs);

  ret = pthread_mutex_init(&g_mutex, NULL);
  if (ret != 0)
  {
    log_error("Mutex initialization failed with %d.", ret);
    ret = 1;
    goto exit;
  }

  install_term_signal_handler(SIGTERM, false);
  install_term_signal_handler(SIGINT, true);

  if (!connect_dbus())
  {
    log_error("Failed to connect to D-Bus");
    ret = 1;
    goto destroy_mutex;
  }

  while (!g_quit)
  {
    dbus_connection_read_write_dispatch(g_dbus_connection, 50);
  }

  ret = 0;

  eat_pair_list(&g_new_pairs);
  eat_pair_list(&g_active_pairs);

  if (g_jack_client != NULL)
  {
    jack_deactivate(g_jack_client);
    jack_client_close(g_jack_client);
  }

  disconnect_dbus();
destroy_mutex:
  pthread_mutex_destroy(&g_mutex);
exit:
  return ret;
}

static void jmcore_connect_to_jack(struct dbus_method_call * call_ptr)
{
  int ret;
  const char * jack_client_name;

  if (g_jack_client != NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK server is already connected");
    return;
  }

  g_jack_client = jack_client_open("jmcore", JackNoStartServer, NULL);
  if (g_jack_client == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot connect to JACK server");
    return;
  }

  ret = jack_set_process_callback(g_jack_client, process_callback, NULL);
  if (ret != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK process callback setup failed");
    jack_client_close(g_jack_client);
    g_jack_client = NULL;
    return;
  }

  jack_on_shutdown(g_jack_client, shutdown_callback, NULL);

  ret = jack_activate(g_jack_client);
  if (ret != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK client activation failed");
    jack_client_close(g_jack_client);
    g_jack_client = NULL;
    return;
  }

  jack_client_name = jack_get_client_name(g_jack_client);

  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &jack_client_name);
}

static void jmcore_create(struct dbus_method_call * call_ptr)
{
  dbus_bool_t midi;
  const char * input;
  const char * output;
  struct port_pair * pair_ptr;

  if (g_jack_client == NULL || g_jack_client_died)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK server is not connected");
    goto exit;
  }

  dbus_error_init(&g_dbus_error);
  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_BOOLEAN, &midi,
        DBUS_TYPE_STRING, &input,
        DBUS_TYPE_STRING, &output,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
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

  pair_ptr->input_port = jack_port_register(g_jack_client, input, midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (pair_ptr->input_port == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Port '%s' registration failed.", input);
    goto free_output_name;
  }

  pair_ptr->output_port = jack_port_register(g_jack_client, output, midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (pair_ptr->input_port == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Port '%s' registration failed.", output);
    goto unregister_input_port;
  }

  pair_ptr->midi = midi;

  pair_ptr->active = false;
  pthread_mutex_lock(&g_mutex);
  list_add_tail(&pair_ptr->siblings, &g_new_pairs);
  pthread_mutex_unlock(&g_mutex);

  /* wait pair to become active */
  pthread_mutex_lock(&g_mutex);
  while (!pair_ptr->active)
  {
    if (g_jack_client_died)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK server died.");
      list_del(&pair_ptr->siblings);
      pthread_mutex_unlock(&g_mutex);
      goto unregister_output_port;
    }

    pthread_mutex_unlock(&g_mutex);
    usleep(50);
    pthread_mutex_lock(&g_mutex);
  }
  pthread_mutex_unlock(&g_mutex);

  method_return_new_void(call_ptr);
  goto exit;

unregister_output_port:
  jack_port_unregister(g_jack_client, pair_ptr->output_port);
unregister_input_port:
  jack_port_unregister(g_jack_client, pair_ptr->input_port);
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

  if (g_jack_client == NULL || g_jack_client_died)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK server is not connected");
    return;
  }

  dbus_error_init(&g_dbus_error);
  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &port, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  pthread_mutex_lock(&g_mutex);
  list_for_each(node_ptr, &g_active_pairs)
  {
    pair_ptr = list_entry(node_ptr, struct port_pair, siblings);
    if (strcmp(pair_ptr->input_port_name, port) == 0 ||
        strcmp(pair_ptr->output_port_name, port) == 0)
    {
      goto found;
    }
  }
  pthread_mutex_unlock(&g_mutex);

  lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "port '%s' not found.", port);
  return;

found:
  pair_ptr->active = false;
  pthread_mutex_unlock(&g_mutex);

  /* wait pair to become non-active */
  pthread_mutex_lock(&g_mutex);
  while (!list_empty(&pair_ptr->siblings))
  {
    if (g_jack_client_died)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "JACK server died.");
      pthread_mutex_unlock(&g_mutex);
      if (list_empty(&pair_ptr->siblings))
      {
        free_pair(pair_ptr);
      }
      return;
    }

    pthread_mutex_unlock(&g_mutex);
    usleep(50);
    pthread_mutex_lock(&g_mutex);
  }
  pthread_mutex_unlock(&g_mutex);

  free_pair(pair_ptr);

  method_return_new_void(call_ptr);
}

static void jmcore_exit(struct dbus_method_call * call_ptr)
{
  log_info("Exit command received through D-Bus");
  g_quit = true;
  method_return_new_void(call_ptr);
}

METHOD_ARGS_BEGIN(connect_to_jack, "Connect to JACK server")
  METHOD_ARG_DESCRIBE_OUT("jack_client_name", "s", "JACK client name")
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
  METHOD_DESCRIBE(connect_to_jack, jmcore_connect_to_jack)
  METHOD_DESCRIBE(create, jmcore_create)
  METHOD_DESCRIBE(destroy, jmcore_destroy)
  METHOD_DESCRIBE(exit, jmcore_exit)
METHODS_END

INTERFACE_BEGIN(g_interface, JMCORE_IFACE)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
