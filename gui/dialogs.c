/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file implements dialogs that are not implemented in dedicated files
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

#include "graph_view.h"
#include "gtk_builder.h"
#include "save_project_dialog.h"

static GtkWidget * g_name_dialog;
GtkWidget * g_app_dialog = NULL;

void run_custom_command_dialog(void)
{
  graph_view_handle view;
  guint result;
  GtkEntry * command_entry = GTK_ENTRY(get_gtk_builder_widget("app_command_entry"));
  GtkEntry * name_entry = GTK_ENTRY(get_gtk_builder_widget("app_name_entry"));
  GtkToggleButton * terminal_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_terminal_check_button"));
  GtkToggleButton * level0_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level0"));
  GtkToggleButton * level1_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level1"));
  GtkToggleButton * level2lash_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level2lash"));
  GtkToggleButton * level2js_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level2js"));
  const char * level;

  view = get_current_view();

  gtk_entry_set_text(name_entry, "");
  gtk_entry_set_text(command_entry, "");
  gtk_toggle_button_set_active(terminal_button, FALSE);

  gtk_widget_set_sensitive(GTK_WIDGET(command_entry), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(terminal_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level0_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level1_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level2lash_button), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(level2js_button), is_room_view(view));

  gtk_window_set_focus(GTK_WINDOW(g_app_dialog), GTK_WIDGET(command_entry));
  gtk_window_set_title(GTK_WINDOW(g_app_dialog), _("New application"));

  gtk_widget_show(g_app_dialog);

  result = gtk_dialog_run(GTK_DIALOG(g_app_dialog));
  if (result == 2)
  {
    if (gtk_toggle_button_get_active(level0_button))
    {
      level = LADISH_APP_LEVEL_0;
    }
    else if (gtk_toggle_button_get_active(level1_button))
    {
      level = LADISH_APP_LEVEL_1;
    }
    else if (gtk_toggle_button_get_active(level2lash_button))
    {
      level = LADISH_APP_LEVEL_LASH;
    }
    else if (gtk_toggle_button_get_active(level2js_button))
    {
      level = LADISH_APP_LEVEL_JACKSESSION;
    }
    else
    {
      log_error("unknown level");
      ASSERT_NO_PASS;
      level = LADISH_APP_LEVEL_0;
    }

    log_info("'%s':'%s' %s level '%s'", gtk_entry_get_text(name_entry), gtk_entry_get_text(command_entry), gtk_toggle_button_get_active(terminal_button) ? "terminal" : "shell", level);
    if (!app_run_custom(
          view,
          gtk_entry_get_text(command_entry),
          gtk_entry_get_text(name_entry),
          gtk_toggle_button_get_active(terminal_button),
          level))
    {
      error_message_box(_("Execution failed. I know you want to know more for the reson but currently you can only check the log file."));
    }
  }

  gtk_widget_hide(g_app_dialog);
}

bool name_dialog(const char * title, const char * object, const char * old_name, char ** new_name)
{
  guint result;
  bool ok;
  GtkEntry * entry = GTK_ENTRY(get_gtk_builder_widget("name_entry"));

  gtk_window_set_title(GTK_WINDOW(g_app_dialog), title);

  gtk_widget_show(g_name_dialog);

  gtk_label_set_text(GTK_LABEL(get_gtk_builder_widget("name_label")), object);
  gtk_entry_set_text(entry, old_name);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

  result = gtk_dialog_run(GTK_DIALOG(g_name_dialog));
  ok = result == 2;
  if (ok)
  {
    *new_name = strdup(gtk_entry_get_text(entry));
    if (*new_name == NULL)
    {
      log_error("strdup failed for new name (name_dialog)");
      ok = false;
    }
  }

  gtk_widget_hide(g_name_dialog);

  return ok;
}

void error_message_box(const char * failed_operation)
{
  GtkWidget * dialog;
  dialog = get_gtk_builder_widget("error_dialog");
  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), _("<b><big>Error</big></b>"));
  gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", failed_operation);
  gtk_widget_show(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
}

void menu_request_start_app(void)
{
  run_custom_command_dialog();
}

void init_dialogs(void)
{
  g_name_dialog = get_gtk_builder_widget("name_dialog");
  g_app_dialog = get_gtk_builder_widget("app_dialog");
  ladish_init_save_project_dialog();
}
