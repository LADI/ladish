/* LASH Control Panel
 * Copyright (C) 2005 Dave Robillard <drobilla@connect.carleton.ca>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_H__
#define __PANEL_H__

#include <gtk/gtk.h>
#include <lash/lash.h>


typedef struct _panel panel_t;

enum
{
	PROJECT_PROJECT_COLUMN,
    PROJECT_NUM_COLUMNS
};


struct _panel
{
	lash_client_t* lash_client;

	GtkListStore* projects;
	
	GtkWidget* window;
	GtkWidget* status_bar;
	GtkWidget* project_notebook;
};

panel_t* panel_create (lash_client_t * client);



#endif /* __PANEL_H__ */
