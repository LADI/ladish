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

#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <time.h>
#include <alloca.h>

#include <lash/lash.h>

#include "server.h"
#include "alsa_mgr.h"
#include "alsa_client.h"
#include "alsa_patch.h"
#include "alsa_fport.h"

#define BACKUP_TIMEOUT ((time_t)(30))

void * alsa_mgr_event_run (void * data);
static void alsa_mgr_new_client_port (alsa_mgr_t * alsa_mgr, uuid_t client_id, unsigned char port);


static void
alsa_mgr_init_alsa (alsa_mgr_t * alsa_mgr)
{
  int err;
  snd_seq_port_info_t * port_info;
  
  /* open the alsa connection */
  err = snd_seq_open (&alsa_mgr->seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
  if (err)
    {
      fprintf (stderr, "%s: error opening alsa sequencer, aborting: %s\n",
               __FUNCTION__, snd_strerror (err));
      abort ();
    }
  
  err = snd_seq_set_client_name (alsa_mgr->seq, "LASH Server");
  if (err)
    {
      fprintf (stderr, "%s: could not set alsa client name: %s\n",
               __FUNCTION__, snd_strerror (err));
    }
  
  printf ("Opened ALSA sequencer with client ID %d\n", snd_seq_client_id (alsa_mgr->seq));
  
  
  /* make our system announcement receiever port */
  snd_seq_port_info_alloca (&port_info);
  snd_seq_port_info_set_name (port_info, "System Announcement Reciever");
  snd_seq_port_info_set_capability (port_info,
    SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_NO_EXPORT);
  snd_seq_port_info_set_type (port_info, SND_SEQ_PORT_TYPE_APPLICATION); /*|SND_SEQ_PORT_TYPE_SPECIFIC); */
  
  err = snd_seq_create_port (alsa_mgr->seq, port_info);
  if (err)
    {
      fprintf (stderr, "%s: error creating alsa port, aborting: %s\n",
               __FUNCTION__, snd_strerror (err));
      abort ();
    }

  
  /* subscribe the port to the system announcer */
  err = snd_seq_connect_from (alsa_mgr->seq,
                              snd_seq_port_info_get_port (port_info),
                              SND_SEQ_CLIENT_SYSTEM,          /* the "System" kernel client */
                              SND_SEQ_PORT_SYSTEM_ANNOUNCE);  /* the "Announce" port */
  if (err)
    {
      fprintf (stderr, "%s: could not connect to system announcer port, aborting: %s\n",
               __FUNCTION__, snd_strerror (err));
      abort ();
    }
}

alsa_mgr_t *
alsa_mgr_new (server_t * server)
{
  alsa_mgr_t * alsa_mgr;
 
  alsa_mgr = lash_malloc0 (sizeof (alsa_mgr_t));
  
  alsa_mgr->server = server;
  
  pthread_mutex_init (&alsa_mgr->lock, NULL);
  
  alsa_mgr_init_alsa (alsa_mgr);
  
  pthread_create (&alsa_mgr->event_thread, NULL, alsa_mgr_event_run, alsa_mgr);
  
  return alsa_mgr;
}

void
alsa_mgr_destroy (alsa_mgr_t * alsa_mgr)
{
  int err;

  alsa_mgr->quit = 1;
  
  LASH_PRINT_DEBUG ("stopping");
  err = pthread_join (alsa_mgr->event_thread, NULL);
  if (err)
    {
      fprintf (stderr, "%s: error joining event thread: %s\n", __FUNCTION__, strerror (errno));
      return;
    }
  LASH_PRINT_DEBUG ("stopped");

  err = snd_seq_close (alsa_mgr->seq);
  if (err)
    fprintf (stderr, "%s: error closing alsa sequencer: %s\n", __FUNCTION__, snd_strerror (err));

  /* FIXME: free data */  

}

/* CLIENT IS LOCKED ON RETURN */
static alsa_client_t *
alsa_mgr_get_client (alsa_mgr_t * alsa_mgr, uuid_t id)
{
  lash_list_t * list;
  alsa_client_t * client;
  
  for (list = alsa_mgr->clients; list; list = lash_list_next (list))
    {
      client = (alsa_client_t *) list->data;
      
      if (uuid_compare (client->id, id) == 0)
        {
          return client;
        }
    }
  
  return NULL;
}

static void
alsa_mgr_check_client_fports (alsa_mgr_t * alsa_mgr, uuid_t client_id)
{
  lash_list_t * list;
  alsa_fport_t * fport;
  unsigned char alsa_id;
  alsa_client_t * client;

  LASH_PRINT_DEBUG ("start");
  
  client = alsa_mgr_get_client (alsa_mgr, client_id);
  if (!client)
    return;
  
  alsa_id = alsa_client_get_client_id (client);
  
  for (list = alsa_mgr->foreign_ports; list;)
    {
      fport = (alsa_fport_t *) list->data;
      list = lash_list_next (list);
      
      if (alsa_fport_get_client (fport) == alsa_id)
        {
          LASH_DEBUGARGS ("resuming previously foreign port %d on client %d",
                         alsa_fport_get_port (fport), alsa_fport_get_client (fport));
          
          alsa_mgr_new_client_port (alsa_mgr, client_id, alsa_fport_get_port (fport));
          
          LASH_DEBUGARGS ("foreign port %d on client %d resumed",
                         alsa_fport_get_port (fport), alsa_fport_get_client (fport));
          
          alsa_mgr->foreign_ports = lash_list_remove (alsa_mgr->foreign_ports, fport);
          alsa_fport_destroy (fport);
        }
    }

  LASH_PRINT_DEBUG ("end");
}

void
alsa_mgr_add_client (alsa_mgr_t * alsa_mgr, uuid_t id,
                     unsigned char alsa_client_id, lash_list_t * alsa_patches)
{
  alsa_client_t * client;

  LASH_PRINT_DEBUG ("start");
  
  client = alsa_client_new ();
  alsa_client_set_client_id (client, alsa_client_id);
  alsa_client_set_id (client, id);
  client->old_patches = alsa_patches;
  
  alsa_mgr->clients = lash_list_append (alsa_mgr->clients, client);
  
#ifdef LASH_DEBUG
  {
    lash_list_t * list;
    alsa_patch_t * patch;
    LASH_DEBUGARGS ("added client with alsa client id '%d' and patches:", alsa_client_id);
    
    for (list = alsa_patches; list; list = lash_list_next (list))
      {
        patch = (alsa_patch_t *) list->data;
        
	LASH_DEBUGARGS ("  %s", alsa_patch_get_desc (patch));
      }
  }
#endif
  
  alsa_mgr_check_client_fports (alsa_mgr, id);

  LASH_PRINT_DEBUG ("end");
}

static alsa_client_t *
alsa_mgr_find_client (alsa_mgr_t * alsa_mgr, uuid_t id)
{
  lash_list_t * list;
  alsa_client_t * client;
  
  for (list = alsa_mgr->clients; list; list = lash_list_next (list))
    {
      client = (alsa_client_t *) list->data;
      
      if (uuid_compare (client->id, id) == 0)
        return client;
    }
  
  return NULL;
}

lash_list_t *
alsa_mgr_remove_client	 (alsa_mgr_t * alsa_mgr, uuid_t id)
{
  alsa_client_t * client;
  lash_list_t * backup_patches;
  
  client = alsa_mgr_find_client (alsa_mgr, id);
  if (client)
    {
      alsa_mgr->clients = lash_list_remove (alsa_mgr->clients, client);
    }
  
  if (!client)
    {
      LASH_PRINT_DEBUG ("could not remove unknown client");
      return NULL;
    }
  
  LASH_DEBUGARGS ("removed client with alsa client id %d", client->client_id);
  
  backup_patches = client->backup_patches;
  client->backup_patches = NULL;
  alsa_client_destroy (client);
  
  return backup_patches;
}

lash_list_t *
alsa_mgr_get_client_patches (alsa_mgr_t * alsa_mgr, uuid_t id)
{
  alsa_client_t * client;
  lash_list_t * patches;
  lash_list_t * node;
  lash_list_t * uniq_patches = NULL;
  lash_list_t * uniq_node;
  alsa_patch_t * patch;
  alsa_patch_t * uniq_patch;
  
  client = alsa_mgr_find_client (alsa_mgr, id);
  if (!client)
    {
      LASH_PRINT_DEBUG ("couldn't get patches for unknown unknown client");
      return NULL;
    }
  
  patches = alsa_client_dup_patches (client);
  
  for (node = patches; node; node = lash_list_next (node))
    {
      patch = (alsa_patch_t *) node->data;
      
      alsa_patch_set (patch, alsa_mgr->clients);

      for (uniq_node = uniq_patches; uniq_node; uniq_node = lash_list_next (uniq_node))
	{
	  uniq_patch = (alsa_patch_t *) uniq_node->data;

	  if (alsa_patch_compare (patch, uniq_patch) == 0)
	    break;
	}

      if (uniq_node)
	alsa_patch_destroy (patch);
      else
	uniq_patches = lash_list_append (uniq_patches, patch);
    }

  lash_list_free (patches);
  
  return uniq_patches;
}

static int
alsa_mgr_resume_patch (alsa_mgr_t * alsa_mgr, alsa_patch_t * patch)
{
  int err;
  snd_seq_port_subscribe_t * sub;
  
  sub = patch->sub;

  err = snd_seq_subscribe_port (alsa_mgr->seq, sub);
  if (err)
    {
      LASH_DEBUGARGS ("could not (yet?) resume alsa subscription '%s': %s\n",
                    alsa_patch_get_desc (patch), snd_strerror (err));
    }
  else
    {
      printf ("Connected ALSA port %d on client %d to port %d on client %d\n",
	      snd_seq_port_subscribe_get_sender(sub)->port,
	      snd_seq_port_subscribe_get_sender(sub)->client,
	      snd_seq_port_subscribe_get_dest(sub)->port,
	      snd_seq_port_subscribe_get_dest(sub)->client);
    }
  
  return err;
}

static void
alsa_mgr_new_client_port (alsa_mgr_t * alsa_mgr, uuid_t client_id, unsigned char port)
{
  alsa_client_t * client;
  lash_list_t * list;
  alsa_patch_t * patch;
  alsa_patch_t * unset_patch;
  int err;
  
  LASH_PRINT_DEBUG ("attempting to resume old patches for client");
  
  client = alsa_mgr_get_client (alsa_mgr, client_id);
  if (!client)
    return;
  
  for (list = client->old_patches; list;)
    {
      patch = (alsa_patch_t *) list->data;
      list = lash_list_next (list);

      
      unset_patch = alsa_patch_dup (patch);
      err = alsa_patch_unset (unset_patch, alsa_mgr->clients);
      if (err)
        {
	  LASH_DEBUGARGS ("could not resume patch '%s' because the other client has not registered\n",
			 alsa_patch_get_desc (unset_patch));
          client->old_patches = lash_list_remove (client->old_patches, patch);
          alsa_patch_destroy (patch);
          alsa_patch_destroy (unset_patch);
          continue;
        }
      
      
      if ( (client->client_id == alsa_patch_get_src_client (unset_patch) &&
    	                 port == alsa_patch_get_src_port (unset_patch)     ) ||
           (client->client_id == alsa_patch_get_dest_client (unset_patch) &&
	                 port == alsa_patch_get_dest_port (unset_patch)     ))
        {
          LASH_DEBUGARGS ("attempting to resume patch for client with alsa client id %d", client->client_id);
          err = alsa_mgr_resume_patch (alsa_mgr, unset_patch);
	  if (!err)
	    {
	      client->old_patches = lash_list_remove (client->old_patches, patch);
	      alsa_patch_destroy (patch);
	    }
        }

      alsa_patch_destroy (unset_patch);
    }
    

  LASH_DEBUGARGS ("resumed all patches for client with alsa client id %d", client->client_id);
  
}

void
alsa_mgr_new_port (alsa_mgr_t * alsa_mgr, unsigned char client, unsigned char port)
{
  lash_list_t * list;
  alsa_client_t * alsa_client;
  uuid_t id;
  
  for (list = alsa_mgr->clients; list; list = lash_list_next (list))
    {
      alsa_client = (alsa_client_t *) list->data;
      
      if (alsa_client_get_client_id (alsa_client) == client)
        break;
    }
  
  if (!list) /* client wasn't found */
    { /* create a new fport */
      alsa_fport_t * fport;
      
      LASH_DEBUGARGS ("adding foreign port %d on client %d", port, client);
      
      fport = alsa_fport_new_with_all (client, port);
      
      alsa_mgr->foreign_ports = lash_list_append (alsa_mgr->foreign_ports, fport);
      
      return;
    }
  
  alsa_client_get_id (alsa_client, id);

  alsa_mgr_new_client_port (alsa_mgr, id, port);
}

/* check the foreign ports */
void
alsa_mgr_port_removed (alsa_mgr_t * alsa_mgr, unsigned char client, unsigned char port)
{
  lash_list_t * list;
  alsa_fport_t * fport;
  
  for (list = alsa_mgr->foreign_ports; list; list = lash_list_next (list))
    {
      fport = (alsa_fport_t *) list->data;
      
      if (fport->client == client && fport->port == port)
        {
          /* remove the fport */
          
          LASH_DEBUGARGS ("removing foreign port with port %d and client %d", client, port);
          
          alsa_mgr->foreign_ports = lash_list_remove (alsa_mgr->foreign_ports, fport);
          
          alsa_fport_destroy (fport);
          
          return;
        }
    }
}

void
alsa_mgr_get_alsa_patches_with_port (alsa_mgr_t * alsa_mgr,
                                     alsa_client_t * client,
                                     snd_seq_port_info_t * port_info,
                                     int type)
{
  snd_seq_query_subscribe_t * query_base;
  alsa_patch_t * patch;
  
  snd_seq_query_subscribe_alloca (&query_base);
  snd_seq_query_subscribe_set_root (query_base, snd_seq_port_info_get_addr (port_info));
  snd_seq_query_subscribe_set_type (query_base, type);
  snd_seq_query_subscribe_set_index (query_base, 0);
  
  while (snd_seq_query_port_subscribers(alsa_mgr->seq, query_base) >= 0)
    {
       patch = alsa_patch_new_with_query (query_base);
       client->patches = lash_list_append (client->patches, patch);
       if (type == SND_SEQ_QUERY_SUBS_WRITE)
         alsa_patch_switch_clients (patch);
       
       snd_seq_query_subscribe_set_index(query_base, snd_seq_query_subscribe_get_index(query_base) + 1);
    }
}

void
alsa_mgr_redo_client_patches (alsa_mgr_t * alsa_mgr, alsa_client_t * client)
{
  snd_seq_port_info_t * port_info;
  
  snd_seq_port_info_alloca (&port_info);
  snd_seq_port_info_set_client (port_info, alsa_client_get_client_id (client));
  snd_seq_port_info_set_port (port_info, -1);
  
  while (snd_seq_query_next_port(alsa_mgr->seq, port_info) >= 0)
    {
      alsa_mgr_get_alsa_patches_with_port (alsa_mgr, client, port_info, SND_SEQ_QUERY_SUBS_READ);
      alsa_mgr_get_alsa_patches_with_port (alsa_mgr, client, port_info, SND_SEQ_QUERY_SUBS_WRITE);
    }
  
#ifdef LASH_DEBUG
  {
    lash_list_t * list;
    alsa_patch_t * patch;
    
    list = client->patches;
    if (list)
      {
        LASH_DEBUGARGS ("got alsa patches for client with alsa client id %d:", alsa_client_get_client_id (client));
        for (; list; list = lash_list_next (list))
          {
            patch = (alsa_patch_t *) list->data;
            
	    LASH_DEBUGARGS ("  %s", alsa_patch_get_desc (patch));
          }
      }
    else
      {
        LASH_DEBUGARGS ("no alsa patches for client with alsa client id %d", alsa_client_get_client_id (client));
      }
  }
#endif
}


void
alsa_mgr_redo_patches (alsa_mgr_t * alsa_mgr)
{
  lash_list_t * list;
  alsa_client_t * client;
  
  LASH_PRINT_DEBUG ("start");
  
  for (list = alsa_mgr->clients; list; list = lash_list_next (list))
    {
      LASH_DEBUGARGS ("list: %p", (void *) list);
      client = (alsa_client_t *) list->data;
      
      alsa_client_free_patches (client);
      alsa_mgr_redo_client_patches (alsa_mgr, client);
    }
  LASH_PRINT_DEBUG ("end");
}

void
alsa_mgr_recieve_event (alsa_mgr_t * alsa_mgr)
{
  snd_seq_event_t* ev;
  int err;
  
  err = snd_seq_event_input (alsa_mgr->seq, &ev);
  
  switch (ev->type)
    {
    case SND_SEQ_EVENT_PORT_START:
      LASH_PRINT_DEBUG ("new port");
      alsa_mgr_new_port (alsa_mgr, ev->data.addr.client, ev->data.addr.port);
      break;
    case SND_SEQ_EVENT_PORT_EXIT:
      alsa_mgr_port_removed (alsa_mgr, ev->data.addr.client, ev->data.addr.port);
      break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
      alsa_mgr_redo_patches (alsa_mgr);
      break;
    }
}

void
alsa_mgr_backup_patches (alsa_mgr_t * alsa_mgr)
{
  static time_t last_backup = 0;
  time_t now;
  lash_list_t * list, * patches;
  alsa_client_t * client;
  
  now = time (NULL);
  
  if (now - last_backup < BACKUP_TIMEOUT)
    return;
    
  for (list = alsa_mgr->clients; list; list = lash_list_next (list))
    {
      client = (alsa_client_t *) list->data;
      
      alsa_client_free_backup_patches (client);
      client->backup_patches = alsa_client_dup_patches (client);
      
      for (patches = client->backup_patches; patches; patches = lash_list_next (patches))
        alsa_patch_set ((alsa_patch_t *) patches->data, alsa_mgr->clients);
    }
  
  last_backup = now;
}

void *
alsa_mgr_event_run (void * data)
{
  alsa_mgr_t * alsa_mgr;
  int err;
  struct pollfd * pfds;
  int nfds, i;
  unsigned short * revents;
  
  alsa_mgr = (alsa_mgr_t *) data;
  
  nfds = snd_seq_poll_descriptors_count(alsa_mgr->seq, POLLIN);
  pfds = lash_malloc (sizeof (struct pollfd) * nfds);
  revents = lash_malloc (sizeof (unsigned short) * nfds);
  snd_seq_poll_descriptors (alsa_mgr->seq, pfds, nfds, POLLIN);
  
  while (!alsa_mgr->quit)
    {
      err = poll (pfds, nfds, 1000);
      if (err == -1)
        {
          if (errno == EINTR)
            continue;
          
          fprintf (stderr, "%s: error polling alsa sequencer: %s\n",
                   __FUNCTION__, strerror (errno));
          continue;
        }
      
      if (alsa_mgr->quit)
        break;

      alsa_mgr_lock (alsa_mgr);      
      err = snd_seq_poll_descriptors_revents (alsa_mgr->seq, pfds, nfds, revents);
      if (err)
        {
          fprintf (stderr, "%s: error getting alsa sequencer poll revents: %s\n",
                   __FUNCTION__, snd_strerror (err));
          continue;
        }

      for (i = 0; i < nfds; i++)
        {
          if (revents[i] > 0)
            alsa_mgr_recieve_event (alsa_mgr);
        }
      
      
      alsa_mgr_backup_patches (alsa_mgr);

      alsa_mgr_unlock (alsa_mgr);
    }
  
  free (revents);
  free (pfds);
  
  LASH_PRINT_DEBUG ("finished");
  return NULL;
}

void
alsa_mgr_lock (alsa_mgr_t * alsa_mgr)
{
  pthread_mutex_lock (&alsa_mgr->lock);
}

void
alsa_mgr_unlock (alsa_mgr_t * alsa_mgr)
{
  pthread_mutex_unlock (&alsa_mgr->lock);
}



/* EOF */

