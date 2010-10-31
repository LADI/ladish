/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to alsapid functionality
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

#ifndef ALSAPID_H__0A27F284_7538_4791_8023_0FBED929EAF3__INCLUDED
#define ALSAPID_H__0A27F284_7538_4791_8023_0FBED929EAF3__INCLUDED

#include <stdbool.h>

void alsapid_compose_src_link(int alsa_client_id, char * buffer);
void alsapid_compose_dst_link(char * buffer);
bool alsapid_get_pid(int alsa_client_id, pid_t * pid_ptr);

#endif /* #ifndef ALSAPID_H__0A27F284_7538_4791_8023_0FBED929EAF3__INCLUDED */
