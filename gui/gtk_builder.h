/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface to the GtkBuilder helpers
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

#ifndef GTK_BUILDER_H__E2BF7CFC_1B04_4160_9165_A1B433C6B3C2__INCLUDED
#define GTK_BUILDER_H__E2BF7CFC_1B04_4160_9165_A1B433C6B3C2__INCLUDED

bool init_gtk_builder(void);
void uninit_gtk_builder(void);
GtkWidget * get_gtk_builder_widget(const char * name);

#endif /* #ifndef GTK_BUILDER_H__E2BF7CFC_1B04_4160_9165_A1B433C6B3C2__INCLUDED */
