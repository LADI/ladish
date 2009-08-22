/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface of the world tree widget
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

#ifndef WORLD_TREE_H__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED
#define WORLD_TREE_H__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED

#include "graph_view.h"

void world_tree_init(void);

void world_tree_add(graph_view_handle view, bool force_activate);
void world_tree_remove(graph_view_handle view);

#endif // #ifndef WORLD_TREE_H__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED
