/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the helper functionality for accessing
 * LASH clients through D-Bus
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

#ifndef LASH_CLIENT_PROXY_H__EE7D1E7B_D2AA_405D_BE28_3C4F0FE26E19__INCLUDED
#define LASH_CLIENT_PROXY_H__EE7D1E7B_D2AA_405D_BE28_3C4F0FE26E19__INCLUDED

#include "common.h"

bool lash_client_proxy_quit(const char * dest);
bool lash_client_proxy_save(const char * dest, const char * app_dir);
bool lash_client_proxy_restore(const char * dest, const char * app_dir);

#endif /* #ifndef LASH_CLIENT_PROXY_H__EE7D1E7B_D2AA_405D_BE28_3C4F0FE26E19__INCLUDED */
