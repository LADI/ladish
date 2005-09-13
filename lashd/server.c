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

#define _GNU_SOURCE

#include <signal.h>
#include <assert.h>
#include <uuid/uuid.h>

#include <lash/loader.h>
#include <lash/internal_headers.h>

#include "server.h"
#include "project.h"
#include "client_event.h"

void server_create_loader (server_t * server);
void server_set_loader    (server_t * server, loader_t * loader);
void server_set_ui_project (server_t * server, const char * project);
void server_set_default_dir (server_t * server, const char * default_dir);


server_t *
server_new (const char * default_dir)
{
  server_t * server;
  server = lash_malloc0 (sizeof (server_t));

  pthread_mutex_init (&server->server_events_lock, NULL);
  pthread_cond_init (&server->server_event_cond, NULL);
  server->default_dir = lash_strdup (default_dir);
  
  server_create_loader (server);

  LASH_PRINT_DEBUG ("starting jack, alsa and comm threads");  
  server->jack_mgr = jack_mgr_new (server);
  server->alsa_mgr = alsa_mgr_new (server);
  server->conn_mgr = conn_mgr_new (server);
  if (!server->conn_mgr)
    exit (1);
  LASH_PRINT_DEBUG ("threads started");
  
  return server;
}

void
server_destroy (server_t * server)
{
  lash_list_t * node;

  for (node = server->projects; node; node = lash_list_next (node))
    project_destroy ((project_t *) node->data);
  lash_list_free (server->projects);

  LASH_PRINT_DEBUG ("destroying connection manager");  
  conn_mgr_destroy (server->conn_mgr);
  LASH_PRINT_DEBUG ("destroying alsa manager");
  alsa_mgr_lock (server->alsa_mgr);
  alsa_mgr_destroy (server->alsa_mgr);
  LASH_PRINT_DEBUG ("destroying jack manager");
  jack_mgr_lock (server->jack_mgr);
  jack_mgr_destroy (server->jack_mgr);

  LASH_PRINT_DEBUG ("destroying loader");
  signal (SIGCHLD, SIG_IGN);
  loader_destroy (server->loader);

  free (server->default_dir);
  free (server);

  LASH_PRINT_DEBUG ("server destroyed");
}

void
server_create_loader  (server_t * server)
{
  loader_t * loader;
  
  if (server->loader)
    {
      loader_destroy (server->loader);
      server->loader = NULL;
      server->loader_quit = 0;
    }
  
  loader = loader_new ();
  if (!loader)
    {
      fprintf (stderr, "%s: could not create loader; aborting\n", __FUNCTION__);
      abort ();
    }

  loader_fork (loader);
  
  server_set_loader (server, loader);
}

void
server_set_loader (server_t * server, loader_t * loader)
{
  server->loader = loader;
}

conn_mgr_t *
server_get_conn_mgr (server_t * server)
{
  return server->conn_mgr;
}

const char *
server_get_default_dir (server_t * server)
{
  return server->default_dir;
}

int
server_get_loader_quit (server_t * server)
{
  return server->loader_quit;
}

void
server_set_loader_quit (server_t * server, int quit)
{
  server->loader_quit = quit;
}

void
server_send_event (server_t * server, server_event_t * server_event)
{
  LASH_DEBUGARGS ("sending server event with type %d", server_event->type);
  pthread_mutex_lock (&server->server_events_lock);
  server->server_events = lash_list_append (server->server_events, server_event);
  pthread_mutex_unlock (&server->server_events_lock);
  pthread_cond_signal (&server->server_event_cond);
}


/*****************************
 ******* server stuff ********
 *****************************/

project_t *
server_find_project_by_name (server_t * server, const char * project_name)
{
  lash_list_t * node;
  project_t * project;

  if (!project_name)
    return NULL;
  
  for (node = server->projects; node; node = lash_list_next (node))
    {
      project = (project_t *) node->data;
      
      if (strcmp (project->name, project_name) == 0)
        return project;
    }
  
  return NULL;
}

project_t *
server_find_project_by_conn_id (server_t * server, unsigned long conn_id)
{
  lash_list_t * list, * client_list;
  project_t * project;
  client_t * client;
  
  list = server->projects;
  while (list)
    {
      project = (project_t *) list->data;
      
      client_list = project->clients;
      while (client_list)
        {
          client = (client_t *) client_list->data;
          
          if (client->conn_id == conn_id)
            return project;
          
          client_list = client_list->next;
        }
      
      list = list->next;
    }
  
  return NULL;
}

int
find_client_in_list_by_conn_id (lash_list_t * client_list, unsigned long conn_id,
                                       client_t ** client_ptr)
{
  client_t * client;

  while (client_list)
    {
          client = (client_t *) client_list->data;
          
          if (client->conn_id == conn_id)
            {
              if (client_ptr)
                *client_ptr = client;
                
              return 1;
            }
          
          client_list = client_list->next;
    }

      return 0;
}

void
server_find_project_and_client_by_conn_id (server_t * server, unsigned long conn_id,
                                           project_t ** project_ptr, client_t ** client_ptr)
{
  lash_list_t * list, * client_list;
  project_t * project;
  int found;
  
  list = server->projects;
  while (list)
    {
      project = (project_t *) list->data;
      
      client_list = project->clients;

      found = find_client_in_list_by_conn_id (project->clients, conn_id, client_ptr);

      if (found)
        {
          if (project_ptr)
            *project_ptr = project;

	  return;
        }
      
      list = list->next;
    }

  found = find_client_in_list_by_conn_id (server->interfaces, conn_id, client_ptr);
  if (found)
    {
      if (project_ptr)
        *project_ptr = NULL;

      return;
    }
  
  if (client_ptr)
    *client_ptr = NULL;
    
  if (project_ptr)
    *project_ptr = NULL;
}

client_t *
server_find_client_by_conn_id (server_t * server, unsigned long conn_id)
{
  lash_list_t * list;
  project_t * project;
  client_t * client;
  
  project = server_find_project_by_conn_id (server, conn_id);
  
  if (!project)
    return NULL;
  
  list = project->clients;
  while (list)
    {
      client = (client_t *) list->data;
      
      if (client->conn_id == conn_id)
        return client;
      
      list = list->next;
    }
  
  return NULL;
}

void
server_notify_interfaces (project_t * project, client_t * client, enum LASH_Event_Type type, const char * str)
{
  lash_list_t * node;
  lash_event_t * event;
  client_t * interface;

  for (node = project->server->interfaces; node; node = lash_list_next (node))
    {
      interface = (client_t *) node->data;
 
      event = lash_event_new_with_type (type);
      lash_event_set_project (event, project->name);
      if (client)
        lash_event_set_client_id (event, client->id);
      lash_event_set_string (event, str);
 
      conn_mgr_send_client_lash_event (project->server->conn_mgr, interface->conn_id, event);
    }
}

const char *
server_create_new_project_name (server_t * server)
{
  static char new_name[16];
  int num = 1;
  
  do
    {
      sprintf (new_name, "project-%d", num);
      num++;
      LASH_DEBUGARGS ("trying project name '%s', dir '%s'", new_name,
		     lash_get_fqn (lash_get_fqn (getenv ("HOME"), server->default_dir), new_name));
    }
  while (project_name_exists (server->projects, new_name) ||
         access (lash_get_fqn (lash_get_fqn (getenv ("HOME"), server->default_dir), new_name), F_OK) == 0);

  return new_name;
}

project_t *
server_create_new_project (server_t * server, const char * name)
{
  project_t * project;

  LASH_PRINT_DEBUG ("creating new project");
  
  project = project_new (server);
  
  if (!name || !strlen (name))
    { /* create a project name */
      LASH_PRINT_DEBUG ("creating new name for project");
      project_set_name (project, server_create_new_project_name (server));
    }
  else
    project_set_name (project, name);

  LASH_DEBUGARGS ("new project's name: %s", project->name);
  
  project_set_directory (project,
			 lash_get_fqn (lash_get_fqn (getenv ("HOME"),
						   server->default_dir),
				      project->name));

  lash_create_dir (project->directory);
  
  printf ("Created project %s in directory %s\n", project->name,
	  project->directory);
  
  server->projects = lash_list_append (server->projects, project);

  server_notify_interfaces (project, NULL, LASH_Project_Add, project->name);
  server_notify_interfaces (project, NULL, LASH_Project_Dir, project->directory);
  
  return project;
}

static void
server_close_project (server_t * server, project_t * project)
{
  server->projects = lash_list_remove (server->projects, project);

  server_notify_interfaces (project, NULL, LASH_Project_Remove, project->name);

  project_destroy (project);
}

void
server_add_interface (server_t * server, client_t * interface)
{
  lash_event_t * event;
  lash_list_t * pnode, * clnode;
  project_t * project;
  client_t * client;

  server->interfaces = lash_list_append (server->interfaces, interface);

  /* tell the interface about all of our projects and clients */
  for (pnode = server->projects; pnode; pnode = lash_list_next (pnode))
    {
      project = (project_t *) pnode->data;

      event = lash_event_new_with_type (LASH_Project_Add);
      lash_event_set_string (event, project->name);
      conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);

      event = lash_event_new_with_type (LASH_Project_Dir);
      lash_event_set_project (event, project->name);
      lash_event_set_string (event, project->directory);
      conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);

      for (clnode = project->clients; clnode; clnode = lash_list_next (clnode))
	{
	  client = (client_t *) clnode->data;

	  event = lash_event_new_with_type (LASH_Client_Add);
	  lash_event_set_project (event, project->name);
	  lash_event_set_client_id (event, client->id);
	  conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);

	  if (client->name)
	    {
	      LASH_DEBUGARGS ("sending client's name to interface: %s", client->name);
	      event = lash_event_new_with_type (LASH_Client_Name);
	      lash_event_set_project (event, project->name);
	      lash_event_set_client_id (event, client->id);
	      lash_event_set_string (event, client->name);
	      conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);
	    }

	  if (client->jack_client_name)
	    {
	      LASH_DEBUGARGS ("sending client's jack client name to interface: %s", client->jack_client_name);
	      event = lash_event_new_with_type (LASH_Jack_Client_Name);
	      lash_event_set_project (event, project->name);
	      lash_event_set_client_id (event, client->id);
	      lash_event_set_string (event, client->jack_client_name);
	      conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);
	    }

	  if (client->alsa_client_id)
	    {
	      char id[2];

	      event = lash_event_new_with_type (LASH_Alsa_Client_ID);
	      lash_event_set_project (event, project->name);
	      lash_event_set_client_id (event, client->id);

	      lash_str_set_alsa_client_id (id, client->alsa_client_id);
	      lash_event_set_string (event, id);

	      LASH_DEBUGARGS ("sending client's alsa client id to interface: %u", client->alsa_client_id);

	      conn_mgr_send_client_lash_event (server->conn_mgr, interface->conn_id, event);
	    }

	}
    }
}

void
server_add_client (server_t * server, client_t * client)
{
  project_t * project;

  LASH_PRINT_DEBUG ("adding client");
  
  if (CLIENT_SERVER_INTERFACE (client))
    {
      LASH_PRINT_DEBUG ("client is an interface; adding to list");
      server_add_interface (server, client);
      return;
    }
  
  project = server_find_project_by_name (server, client->requested_project);
  if (!project)
    { /* try and find an existing project */
      if ((!client->requested_project || !strlen (client->requested_project) )
          && server->projects)
        {
          project = (project_t *) server->projects->data;
        }
      else
	{
	  LASH_PRINT_DEBUG ("creating new project for client");
	  project = server_create_new_project (server, client->requested_project);
	}
    }

  LASH_DEBUGARGS ("adding client to project '%s'", project->name);
  project_add_client (project, client);
}

void
server_event_client_connect (server_t * server, server_event_t * event)
{
  client_t * client;
  
  client = client_new ();
  client->conn_id = event->conn_id;
  client_set_from_connect_params (client, event->data.lash_connect_params);
  
  server_add_client (server, client);
}

void
server_event_client_disconnect (server_t * server, server_event_t * event)
{
  client_t * client;
  project_t * project;
  lash_list_t * jack_patches = NULL;
  lash_list_t * alsa_patches = NULL;

  server_find_project_and_client_by_conn_id (server, event->conn_id, &project, &client);
  if (!client)
    {
      fprintf (stderr, "%s: recieved disconnect for connection id %ld, but no client exists for it :/\n",
               __FUNCTION__, event->conn_id);
      return;
    }

  if (CLIENT_SERVER_INTERFACE (client))
    {
      server->interfaces = lash_list_remove (server->interfaces, client);
      client_destroy (client);
      return;
    }
  
  if (client->jack_client_name)
    {
      jack_mgr_lock (server->jack_mgr);
      jack_patches = jack_mgr_remove_client (server->jack_mgr, client->id);
      jack_mgr_unlock (server->jack_mgr);
    }
  
  if (client->alsa_client_id)
    {
      alsa_mgr_lock (server->alsa_mgr);
      alsa_patches = alsa_mgr_remove_client (server->alsa_mgr, client->id);
      alsa_mgr_unlock (server->alsa_mgr);
    }

  project_lose_client (project, client, jack_patches, alsa_patches);
  
  if (!project->clients)
    server_close_project (server, project);
}

void
server_event_client_event (server_t * server, server_event_t * server_event)
{
  lash_event_t *event;
  project_t *project;
  client_t *client, *ref_client;
  
  event = server_event_take_lash_event (server_event);

  
  /* find the project and client */
  server_find_project_and_client_by_conn_id (server, server_event->conn_id, &project, &client);
  if (!client)
    {
      fprintf (stderr, "%s: recieved server event from connection manager for unknown client with connection id %ld\n",
               __FUNCTION__, server_event->conn_id);
      return;
    }

  if (!project)
    {
      if (!CLIENT_SERVER_INTERFACE (client))
	{
	  fprintf (stderr, "%s: recieved server event from client '%s' "
		   "that isn't connected to a project\n", __FUNCTION__,
		   client_get_id_str (client));
	  return;
	}
      
      project = server_find_project_by_name (server, lash_event_get_project (event));

      if (project && !uuid_is_null (event->client_id))
	{
	  ref_client = project_get_client_by_id (project, event->client_id);
	  if (!ref_client)
	    {
	      fprintf (stderr, "%s: recieved event from server interface with non-existant client "
		       "in project '%s'\n", __FUNCTION__, project->name);
	      return;
	    }
	}
    }


  switch (lash_event_get_type (event))
    {
    case LASH_Client_Name:
      if (CLIENT_SERVER_INTERFACE (client))
	server_lash_event_client_name (project, ref_client, client, lash_event_get_string (event));
      else
	server_lash_event_client_name (project, client, client, lash_event_get_string (event));
      break;
    case LASH_Jack_Client_Name:
      if (CLIENT_SERVER_INTERFACE (client))
	server_lash_event_jack_client_name (project, ref_client, client, lash_event_get_string (event));
      else
	server_lash_event_jack_client_name (project, client, client, lash_event_get_string (event));
      break;
    case LASH_Alsa_Client_ID:
      if (CLIENT_SERVER_INTERFACE (client))
	server_lash_event_alsa_client_id (project, ref_client, client, lash_event_get_string (event));
      else
	server_lash_event_alsa_client_id (project, client, client, lash_event_get_string (event));
      break;
    case LASH_Restore_Data_Set:
      server_lash_event_restore_data_set (project, client);
      break;
    case LASH_Save_Data_Set:
      server_lash_event_save_data_set (project, client);
      break;
    case LASH_Restore_File:
      break;
    case LASH_Save_File:
      server_lash_event_save_file (project, client);
      break;
    case LASH_Save:
      project_save (project);
      break;
    case LASH_Quit:
      break;
      
    case LASH_Project_Add:
      server_lash_event_project_add (server, lash_event_get_string (event));
      break;
    case LASH_Project_Remove:
      server_close_project (server, project);
      break;
    case LASH_Project_Dir:
      server_lash_event_project_dir (project, client, lash_event_get_string (event));
      break;
    case LASH_Project_Name:
      server_lash_event_project_name (project, lash_event_get_string (event));
      break;
    case LASH_Client_Remove:
      server_lash_event_client_remove (project, ref_client);
      break;
 /* case LASH_Percentage: */
    
    default:
      fprintf (stderr, "%s: received unknown cca event of type %d\n", __FUNCTION__,
               lash_event_get_type (event));
      break;
    }
  
  lash_event_destroy (event);
}

void
server_event_client_config (server_t * server, server_event_t * event)
{
  client_t * client;
  store_t * store;

  /* find the project and client */
  server_find_project_and_client_by_conn_id (server, event->conn_id, NULL, &client);
  if (!client)
    {
      fprintf (stderr, "%s: recieved server event from connection manager for unknown client with connection id %ld\n",
               __FUNCTION__, event->conn_id);
      return;
    }
  
  if (!CLIENT_CONFIG_DATA_SET (client))
   {
     fprintf (stderr, "%s: client '%s' sent us a config but doesn't describe itself as configured with a data set\n",
              __FUNCTION__, client_get_id_str (client));
     return;
   }
  
  store = client->store;
  
  store_set_config (store, event->data.lash_config);
}

void
server_deal_with_event (server_t * server, server_event_t * event)
{
  switch (event->type)
    {
    case Client_Connect:
      server_event_client_connect (server, event);
      break;
    case Client_Disconnect:
      server_event_client_disconnect (server, event);
      break;
    case Client_Event:
      server_event_client_event (server, event);
      break;
    case Client_Config:
      server_event_client_config (server, event);
      break;
    default:
      fprintf (stderr, "%s: recieved unknown server event of type %d\n",
               __FUNCTION__, event->type);
    }
}

void
server_main (server_t * server)
{
  server_event_t * server_event;
  lash_list_t * server_events, * list;
  struct timespec ts;
  struct timeval now;
  
  while (!server->quit)
    {
      pthread_mutex_lock (&server->server_events_lock);
      server_events = server->server_events;
     
	   if (!server_events)
        {
          gettimeofday(&now, NULL);
          ts.tv_sec = now.tv_sec + 1;
          ts.tv_nsec = now.tv_usec * 1000;
          
          pthread_cond_timedwait (&server->server_event_cond,
				  &server->server_events_lock,
				  &ts);

          server_events = server->server_events;
        }

      server->server_events = NULL;
      pthread_mutex_unlock (&server->server_events_lock);
      
      for (list = server_events; list; list = lash_list_next (list))
        {
          server_event = (server_event_t *) list->data;
	  LASH_DEBUGARGS ("recieved event of type %d", server_event->type);
          server_deal_with_event (server, server_event);
	  server_event_destroy (server_event);
        }
      
      lash_list_free (server_events);
    }
  
  
  
  LASH_PRINT_DEBUG ("finished");
}

/* EOF */

