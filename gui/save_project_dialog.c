/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the save project dialog
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

#include "save_project_dialog.h"
#include "gtk_builder.h"

void ladish_run_save_project_dialog(ladish_room_proxy_handle room)
{
  GtkWidget * dialog;
  GtkEntry * path;
  GtkEntry * name;
  GtkResponseType response;

  dialog = get_gtk_builder_widget("project_save_as_dialog");
  path = GTK_ENTRY(get_gtk_builder_widget("project_save_as_path_entry"));
  name = GTK_ENTRY(get_gtk_builder_widget("project_save_as_name_entry"));

  gtk_entry_set_text(path, "");
  gtk_entry_set_text(name, "");

  gtk_widget_show(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
  if (response == GTK_RESPONSE_OK)
  {
    log_info("Saving project to '%s' (%s)", gtk_entry_get_text(path), gtk_entry_get_text(name));
    if (!ladish_room_proxy_save_project(room, gtk_entry_get_text(path), gtk_entry_get_text(name)))
    {
      log_error("ladish_room_proxy_save_project() failed.");
    }
  }
}
