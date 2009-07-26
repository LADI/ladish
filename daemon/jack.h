/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  JACK server monitor and control
 *
 *****************************************************************************/

#ifndef JACK_H__1C44BAEA_280C_4235_94AB_839499BDE47F__INCLUDED
#define JACK_H__1C44BAEA_280C_4235_94AB_839499BDE47F__INCLUDED

#include "common.h"

bool
jack_init(
  void);

void
jack_uninit(
  void);

void
jack_conf_container_destroy(
  struct jack_conf_container * container_ptr);

void
jack_conf_parameter_destroy(
  struct jack_conf_parameter * parameter_ptr);

#endif /* #ifndef JACK_H__1C44BAEA_280C_4235_94AB_839499BDE47F__INCLUDED */
