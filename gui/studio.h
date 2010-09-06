/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the studio handling code
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

#ifndef STUDIO_H__5249923A_C9AE_4D74_8B62_EED013F49CB1__INCLUDED
#define STUDIO_H__5249923A_C9AE_4D74_8B62_EED013F49CB1__INCLUDED

#include "common.h"

#define STUDIO_STATE_UNKNOWN    0
#define STUDIO_STATE_UNLOADED   1
#define STUDIO_STATE_STOPPED    2
#define STUDIO_STATE_STARTED    3
#define STUDIO_STATE_CRASHED    4
#define STUDIO_STATE_NA         5
#define STUDIO_STATE_SICK       6

unsigned int get_studio_state(void);
void set_studio_state(unsigned int state);

bool studio_state_changed(char ** name_ptr_ptr);

bool studio_loaded(void);
void create_studio_view(const char * name);
void destroy_studio_view(void);

void set_studio_callbacks(void);

#endif /* #ifndef STUDIO_H__5249923A_C9AE_4D74_8B62_EED013F49CB1__INCLUDED */
