/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implements the project properties dialog helpers
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

#include "common.h"
#include "gtk_builder.h"
#include "graph_view.h"

void ladish_project_properties_dialog_run(void)
{
  GtkDialog * dialog_ptr;
  GtkEntry * name_entry_ptr;
  GtkEntry * description_entry_ptr;
  GtkTextView * notes_text_view_ptr;
  ladish_room_proxy_handle proxy;
  const char * dir;
  const char * name;
  const char * description;
  const char * notes;
  GtkTextBuffer * notes_buffer;
  GtkTextIter start;
  GtkTextIter end;
  guint result;
  bool ok;

  dialog_ptr = GTK_DIALOG(get_gtk_builder_widget("project_properties_dialog"));
  name_entry_ptr = GTK_ENTRY(get_gtk_builder_widget("project_name"));
  description_entry_ptr = GTK_ENTRY(get_gtk_builder_widget("project_description"));
  notes_text_view_ptr = GTK_TEXT_VIEW(get_gtk_builder_widget("project_notes"));

  proxy = graph_view_get_room(get_current_view());
  ladish_room_proxy_get_project_properties(proxy, &dir, &name, &description, &notes);

  gtk_entry_set_text(name_entry_ptr, name);
  gtk_widget_set_sensitive(GTK_WIDGET(name_entry_ptr), FALSE); /* rename is not implemented yet */

  gtk_entry_set_text(description_entry_ptr, description);
  gtk_window_set_focus(GTK_WINDOW(dialog_ptr), GTK_WIDGET(description_entry_ptr));

  notes_buffer = gtk_text_view_get_buffer(notes_text_view_ptr);
  gtk_text_buffer_set_text(notes_buffer, notes, -1);

  gtk_widget_show(GTK_WIDGET(dialog_ptr));

  result = gtk_dialog_run(dialog_ptr);
  ok = result == 2;
  if (ok)
  {
    if (!ladish_room_proxy_set_project_description(proxy, gtk_entry_get_text(description_entry_ptr)))
    {
      error_message_box("Setting of project description failed, please inspect logs.");
    }
    else
    {
      gtk_text_buffer_get_start_iter(notes_buffer, &start);
      gtk_text_buffer_get_end_iter(notes_buffer, &end);

      if (!ladish_room_proxy_set_project_notes(proxy, gtk_text_buffer_get_text(notes_buffer, &start, &end, FALSE)))
      {
        error_message_box("Setting of project description failed, please inspect logs.");
      }
    }
  }

  gtk_widget_hide(GTK_WIDGET(dialog_ptr));
}
