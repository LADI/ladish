/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to recent projects functionality
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

#ifndef RECENT_PROJECTS_H__51B24E64_1629_43A4_B312_14E84080E68E__INCLUDED
#define RECENT_PROJECTS_H__51B24E64_1629_43A4_B312_14E84080E68E__INCLUDED

#include "common.h"

bool ladish_recent_projects_init(void);
void ladish_recent_projects_uninit(void);

void ladish_recent_project_use(const char * project_path);

extern const struct dbus_interface_descriptor g_iface_recent_items;

#endif /* #ifndef RECENT_PROJECTS_H__51B24E64_1629_43A4_B312_14E84080E68E__INCLUDED */
