/*
 *   LASH
 *    
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_GTK_CLIENT_H__
#define __LASH_GTK_CLIENT_H__

#include <gtk/gtk.h>
#include <lash/lash.h>


typedef struct _gtk_client gtk_client_t;

enum
{
  KEY_COLUMN,
  VALUE_COLUMN,
  N_COLUMNS
};

struct _gtk_client
{
  lash_client_t *  lash_client;
  
  GtkListStore *  configs;
  GtkTextBuffer * file_data;

  GtkWidget *     window;
  GtkWidget *     status_bar;
  GtkWidget *     config_list;
  GtkWidget *     config_editor;
  GtkWidget *     config_editor_key;
  GtkWidget *     config_editor_value;
};

extern const char * filename;
gtk_client_t * gtk_client_create (lash_client_t * client);



#endif /* __LASH_GTK_CLIENT_H__ */
