/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to JACK server monitor and control code
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
