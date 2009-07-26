/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  JACK server monitor and control
 *
 *****************************************************************************/

#include "jack.h"
#include "jack_proxy.h"

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
  struct jack_parameter_variant parameter;
  bool is_set;

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
  assert(context == address);
  dst = (char *)component;

  len = strlen(child) + 1;
  memcpy(dst, child, len);
  dst[len] = 0;

  if (leaf)
  {
    lash_debug("%s (leaf)", path);

    if (!jack_proxy_get_parameter_value(context, &is_set, &parameter))
    {
      lash_error("cannot get value of %s", path);
      return false;
    }

    if (is_set)
    {
      switch (parameter.type)
      {
      case jack_boolean:
        lash_info("%s value is %s (boolean)", path, parameter.value.boolean ? "true" : "false");
        break;
      case jack_string:
        lash_info("%s value is %s (string)", path, parameter.value.string);
        break;
      case jack_byte:
        lash_info("%s value is %u/%c (byte/char)", path, parameter.value.byte, (char)parameter.value.byte);
        break;
      case jack_uint32:
        lash_info("%s value is %u (uint32)", path, (unsigned int)parameter.value.uint32);
        break;
      case jack_int32:
        lash_info("%s value is %u (int32)", path, (signed int)parameter.value.int32);
        break;
      default:
        lash_error("ignoring unknown jack parameter type %d (%s)", (int)parameter.type, path);
      }
    }

    if (parameter.type == jack_string)
    {
      free(parameter.value.string);
    }
  }
  else
  {
    lash_debug("%s (container)", path);

    if (!jack_proxy_read_conf_container(context, context, conf_callback))
    {
      lash_error("cannot read container %s", path);
      return false;
    }
  }

  *dst = 0;

  return true;
}

void
on_jack_server_started(
  void)
{
  char buffer[1024] = "";
  lash_info("JACK server start detected.");

  if (!jack_proxy_read_conf_container(buffer, buffer, conf_callback))
  {
    lash_error("jack_proxy_read_conf_container() failed.");
  }
}

void
on_jack_server_stopped(
  void)
{
  lash_info("JACK server stop detected.");
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
