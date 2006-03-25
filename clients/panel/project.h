/* LASH Control Panel
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <gtk/gtk.h>
#include <lash/lash.h>

typedef struct _project project_t;

enum
{
    CLIENT_NAME_COLUMN,
	CLIENT_ID_COLUMN,
    CLIENT_NUM_COLUMNS
};


struct _project {
	
	char* name;
	char* dir;
	int   page_number;

	lash_client_t* lash_client;
	
	GtkListStore* clients;
	
	GtkWidget* box;
	GtkWidget* tab_label;
	
	GtkWidget*         properties_label;
	GtkWidget*         properties_table;
	GtkWidget*         name_label;
	GtkWidget*         name_entry;
	GtkWidget*         set_name_button;
	GtkWidget*         dir_label;
	GtkWidget*         dir_entry;
	GtkWidget*         set_dir_button;
	GtkWidget*         clients_label;
	GtkWidget*         clients_align_box;
	GtkWidget*         clients_align_label;
	GtkWidget*         clients_list;
	GtkWidget*         clients_list_scroll;
	GtkCellRenderer*   clients_renderer;
	GtkTreeViewColumn* name_column;
	GtkTreeViewColumn* id_column;
	GtkWidget*         clients_button_box;
	GtkWidget*         save_button;
	GtkWidget*         close_button;
};

project_t* project_create(lash_client_t* lash_client, const char* const name);
void       project_destroy(project_t* project);

void project_set_name(project_t* project, const char* const name);
void project_set_dir(project_t* project, const char* const dir);

#endif /* __PROJECT_H__ */
