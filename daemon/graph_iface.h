/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the D-Bus patchbay interface helpers
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

#ifndef PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED
#define PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED

struct graph_implementator
{
  void * this;

  uint64_t (* get_graph_version)(void * this);
};

extern const struct dbus_interface_descriptor g_interface_patchbay;

#endif /* #ifndef PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED */
