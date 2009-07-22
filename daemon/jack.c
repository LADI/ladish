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
