/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Studio object helpers
 *
 *****************************************************************************/

#include "common.h"
#include "jack.h"

bool
studio_create(
  struct studio ** studio_ptr_ptr)
{
  struct studio * studio_ptr;

  lash_info("studio object construct");

  studio_ptr = malloc(sizeof(struct studio));
  if (studio_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct studio");
    return false;
  }

  INIT_LIST_HEAD(&studio_ptr->all_connections);
  INIT_LIST_HEAD(&studio_ptr->all_ports);
  INIT_LIST_HEAD(&studio_ptr->all_clients);
  INIT_LIST_HEAD(&studio_ptr->jack_connections);
  INIT_LIST_HEAD(&studio_ptr->jack_ports);
  INIT_LIST_HEAD(&studio_ptr->jack_clients);
  INIT_LIST_HEAD(&studio_ptr->rooms);
  INIT_LIST_HEAD(&studio_ptr->clients);
  INIT_LIST_HEAD(&studio_ptr->ports);

  studio_ptr->modified = false;
  studio_ptr->persisted = false;
  studio_ptr->jack_conf_obsolete = false;
  studio_ptr->jack_conf_stable = false;

  INIT_LIST_HEAD(&studio_ptr->jack_conf);

  *studio_ptr_ptr = studio_ptr;

  return true;
}

void
studio_destroy(
  struct studio * studio_ptr)
{
  struct list_head * node_ptr;

  while (!list_empty(&studio_ptr->jack_conf))
  {
    node_ptr = studio_ptr->jack_conf.next;
    list_del(node_ptr);
    jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
  }

  free(studio_ptr);
  lash_info("studio object destroy");
}
