/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the GtkAction related code
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

#ifndef ACTION_H__AC7D986C_607B_4143_A637_7CF673F7F3A2__INCLUDED
#define ACTION_H__AC7D986C_607B_4143_A637_7CF673F7F3A2__INCLUDED

#include "common.h"

extern GtkAction * g_clear_xruns_and_max_dsp_action;

void init_actions_and_accelerators(void);
void enable_action(GtkAction * action);
void disable_action(GtkAction * action);

#endif /* #ifndef ACTION_H__AC7D986C_607B_4143_A637_7CF673F7F3A2__INCLUDED */
