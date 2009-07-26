/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  JACK server monitor and control
 *
 *****************************************************************************/

#include "jack.h"
#include "jack_proxy.h"
#include "studio.h"

bool
jack_conf_container_create(
  struct jack_conf_container ** container_ptr_ptr,
  const char * name)
{
  struct jack_conf_container * container_ptr;

  container_ptr = malloc(sizeof(struct jack_conf_container));
  if (container_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct jack_conf_container");
    goto fail;
  }

  container_ptr->name = strdup(name);
  if (container_ptr->name == NULL)
  {
    lash_error("strdup() failed to duplicate \"%s\"", name);
    goto fail_free;
  }

  INIT_LIST_HEAD(&container_ptr->children);
  container_ptr->children_leafs = false;

  *container_ptr_ptr = container_ptr;
  return true;

fail_free:
  free(container_ptr);

fail:
  return false;
}

void
jack_conf_container_destroy(
  struct jack_conf_container * container_ptr)
{
  struct list_head * node_ptr;

  //lash_info("\"%s\" jack_conf_parameter destroy", container_ptr->name);

  if (!container_ptr->children_leafs)
  {
    while (!list_empty(&container_ptr->children))
    {
      node_ptr = container_ptr->children.next;
      list_del(node_ptr);
      jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
    }
  }
  else
  {
    while (!list_empty(&container_ptr->children))
    {
      node_ptr = container_ptr->children.next;
      list_del(node_ptr);
      jack_conf_parameter_destroy(list_entry(node_ptr, struct jack_conf_parameter, siblings));
    }
  }

  free(container_ptr->name);
  free(container_ptr);
}

bool
jack_conf_parameter_create(
  struct jack_conf_parameter ** parameter_ptr_ptr,
  const char * name)
{
  struct jack_conf_parameter * parameter_ptr;

  parameter_ptr = malloc(sizeof(struct jack_conf_parameter));
  if (parameter_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct jack_conf_parameter");
    goto fail;
  }

  parameter_ptr->name = strdup(name);
  if (parameter_ptr->name == NULL)
  {
    lash_error("strdup() failed to duplicate \"%s\"", name);
    goto fail_free;
  }

  *parameter_ptr_ptr = parameter_ptr;
  return true;

fail_free:
  free(parameter_ptr);

fail:
  return false;
}

void
jack_conf_parameter_destroy(
  struct jack_conf_parameter * parameter_ptr)
{
#if 0
  lash_info("jack_conf_parameter destroy");

  switch (parameter_ptr->parameter.type)
  {
  case jack_boolean:
    lash_info("%s value is %s (boolean)", parameter_ptr->name, parameter_ptr->parameter.value.boolean ? "true" : "false");
    break;
  case jack_string:
    lash_info("%s value is %s (string)", parameter_ptr->name, parameter_ptr->parameter.value.string);
    break;
  case jack_byte:
    lash_info("%s value is %u/%c (byte/char)", parameter_ptr->name, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
    break;
  case jack_uint32:
    lash_info("%s value is %u (uint32)", parameter_ptr->name, (unsigned int)parameter_ptr->parameter.value.uint32);
    break;
  case jack_int32:
    lash_info("%s value is %u (int32)", parameter_ptr->name, (signed int)parameter_ptr->parameter.value.int32);
    break;
  default:
    lash_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, parameter_ptr->name);
    break;
  }
#endif

  if (parameter_ptr->parameter.type == jack_string)
  {
    free(parameter_ptr->parameter.value.string);
  }

  free(parameter_ptr->name);
  free(parameter_ptr);
}

bool
jack_init(
  void)
{
  if (!jack_proxy_init())
  {
    return false;
  }

  return true;
}

void
jack_uninit(
  void)
{
  jack_proxy_uninit();
}

void
on_jack_client_appeared(
  uint64_t client_id,
  const char * client_name)
{
  lash_info("JACK client appeared.");
}

void
on_jack_client_disappeared(
  uint64_t client_id)
{
  lash_info("JACK client disappeared.");
}

void
on_jack_port_appeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name)
{
  lash_info("JACK port appeared.");
}

void
on_jack_port_disappeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name)
{
  lash_info("JACK port disappeared.");
}

void
on_jack_ports_connected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  lash_info("JACK ports connected.");
}

void
on_jack_ports_disconnected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  lash_info("JACK ports disconnected.");
}

struct conf_callback_context
{
  char address[1024];
  struct list_head * container_ptr;
  struct studio * studio_ptr;
  struct jack_conf_container * parent_ptr;
};

#define context_ptr ((struct conf_callback_context *)context)

bool
conf_callback(
  void * context,
  bool leaf,
  const char * address,
  char * child)
{
  char path[1024];
  const char * component;
  char * dst;
  size_t len;
  bool is_set;
  struct jack_conf_container * parent_ptr;
  struct jack_conf_container * container_ptr;
  struct jack_conf_parameter * parameter_ptr;

  if (context_ptr->studio_ptr->jack_conf_obsolete)
  {
    context_ptr->studio_ptr = NULL;
    return false;
  }

  assert(context_ptr->studio_ptr);

  parent_ptr = context_ptr->parent_ptr;

  dst = path;
  component = address;
  while (*component != 0)
  {
    len = strlen(component);
    memcpy(dst, component, len);
    dst[len] = ':';
    component += len + 1;
    dst += len + 1;
  }

  strcpy(dst, child);

  /* address always is same buffer as the one supplied through context pointer */
  assert(context_ptr->address == address);
  dst = (char *)component;

  len = strlen(child) + 1;
  memcpy(dst, child, len);
  dst[len] = 0;

  if (leaf)
  {
    lash_debug("%s (leaf)", path);

    if (parent_ptr == NULL)
    {
      lash_error("jack conf parameters can't appear in root container");
      return false;
    }

    if (!parent_ptr->children_leafs)
    {
      if (!list_empty(&parent_ptr->children))
      {
        lash_error("jack conf parameters cant be mixed with containers at same hierarchy level");
        return false;
      }

      parent_ptr->children_leafs = true;
    }

    if (!jack_conf_parameter_create(&parameter_ptr, child))
    {
      lash_error("jack_conf_parameter_create() failed");
      return false;
    }

    if (!jack_proxy_get_parameter_value(context_ptr->address, &is_set, &parameter_ptr->parameter))
    {
      lash_error("cannot get value of %s", path);
      return false;
    }

    if (is_set)
    {
      switch (parameter_ptr->parameter.type)
      {
      case jack_boolean:
        lash_info("%s value is %s (boolean)", path, parameter_ptr->parameter.value.boolean ? "true" : "false");
        break;
      case jack_string:
        lash_info("%s value is %s (string)", path, parameter_ptr->parameter.value.string);
        break;
      case jack_byte:
        lash_info("%s value is %u/%c (byte/char)", path, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
        break;
      case jack_uint32:
        lash_info("%s value is %u (uint32)", path, (unsigned int)parameter_ptr->parameter.value.uint32);
        break;
      case jack_int32:
        lash_info("%s value is %u (int32)", path, (signed int)parameter_ptr->parameter.value.int32);
        break;
      default:
        lash_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, path);
        jack_conf_parameter_destroy(parameter_ptr);
        return false;
      }

      list_add_tail(&parameter_ptr->siblings, &parent_ptr->children);
    }
    else
    {
      jack_conf_parameter_destroy(parameter_ptr);
    }
  }
  else
  {
    lash_debug("%s (container)", path);

    if (parent_ptr != NULL && parent_ptr->children_leafs)
    {
      lash_error("jack conf containers cant be mixed with parameters at same hierarchy level");
      return false;
    }

    if (!jack_conf_container_create(&container_ptr, child))
    {
      lash_error("jack_conf_container_create() failed");
      return false;
    }

    if (parent_ptr == NULL)
    {
      list_add_tail(&container_ptr->siblings, &context_ptr->studio_ptr->jack_conf);
    }
    else
    {
      list_add_tail(&container_ptr->siblings, &parent_ptr->children);
    }

    context_ptr->parent_ptr = container_ptr;

    if (!jack_proxy_read_conf_container(context_ptr->address, context, conf_callback))
    {
      lash_error("cannot read container %s", path);
      return false;
    }

    context_ptr->parent_ptr = parent_ptr;
  }

  *dst = 0;

  return true;
}

#undef context_ptr

void
on_jack_server_started(
  void)
{
  struct conf_callback_context context;

  lash_info("JACK server start detected.");

  if (g_studio_ptr == NULL)
  {
    if (!studio_create(&g_studio_ptr))
    {
      lash_error("failed to create studio object");
      return;
    }
  }

  context.address[0] = 0;
  context.container_ptr = &g_studio_ptr->jack_conf;
  context.studio_ptr = g_studio_ptr;
  context.parent_ptr = NULL;

  if (jack_proxy_read_conf_container(context.address, &context, conf_callback))
  {
    if (g_studio_ptr == context.studio_ptr &&
        !g_studio_ptr->jack_conf_obsolete)
    {
      g_studio_ptr->jack_conf_stable = true;
      lash_info("jack conf successfully retrieved");
      return;
    }
  }
  else
  {
    lash_error("jack_proxy_read_conf_container() failed.");
    g_studio_ptr = NULL;
  }

  studio_destroy(context.studio_ptr);
}

void
on_jack_server_stopped(
  void)
{
  lash_info("JACK server stop detected.");

  if (g_studio_ptr == NULL)
  {
    return;
  }

  g_studio_ptr->jack_conf_obsolete = true;

  if (!g_studio_ptr->persisted)
  {
    studio_destroy(g_studio_ptr);
    g_studio_ptr = NULL;
    return;
  }

  /* TODO: if user wants, restart jack server and reconnect all jack apps to it */
}

void
on_jack_server_appeared(
  void)
{
  lash_info("JACK controller appeared.");
}

void
on_jack_server_disappeared(
  void)
{
  lash_info("JACK controller disappeared.");
}
