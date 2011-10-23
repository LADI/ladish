/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the settings dialog
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

#include "menu.h"
#include "gtk_builder.h"
#include "../proxies/conf_proxy.h"
#include "../daemon/conf.h"

void menu_request_settings(void)
{
  guint result;
  GtkDialog * dialog;
  GtkToggleButton * autostart_studio_button;
  GtkToggleButton * send_notifications_button;
  GtkEntry * shell_entry;
  GtkEntry * terminal_entry;
  GtkSpinButton * js_delay_spin;
  bool autostart;
  bool notify;
  const char * shell;
  const char * terminal;
  unsigned int js_delay;

  autostart_studio_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("settings_studio_autostart_checkbutton"));
  send_notifications_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("settings_send_notifications_checkbutton"));
  shell_entry = GTK_ENTRY(get_gtk_builder_widget("settings_shell_entry"));
  terminal_entry = GTK_ENTRY(get_gtk_builder_widget("settings_terminal_entry"));
  js_delay_spin = GTK_SPIN_BUTTON(get_gtk_builder_widget("settings_js_delay_spin"));

  dialog = GTK_DIALOG(get_gtk_builder_widget("settings_dialog"));

  if (!conf_get_bool(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, &autostart))
  {
    autostart = LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT;
  }

  if (!conf_get_bool(LADISH_CONF_KEY_DAEMON_NOTIFY, &notify))
  {
    notify = LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT;
  }

  if (!conf_get(LADISH_CONF_KEY_DAEMON_SHELL, &shell))
  {
    shell = LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT;
  }

  if (!conf_get(LADISH_CONF_KEY_DAEMON_TERMINAL, &terminal))
  {
    terminal = LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT;
  }

  if (!conf_get_uint(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, &js_delay))
  {
    js_delay = LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT;
  }

  gtk_toggle_button_set_active(autostart_studio_button, autostart);
  gtk_toggle_button_set_active(send_notifications_button, notify);

  gtk_entry_set_text(shell_entry, shell);
  gtk_entry_set_text(terminal_entry, terminal);

  gtk_spin_button_set_range(js_delay_spin, 0, 1000);
  gtk_spin_button_set_increments(js_delay_spin, 1, 2);
  gtk_spin_button_set_value(js_delay_spin, js_delay);

  gtk_widget_show(GTK_WIDGET(dialog));
  result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(GTK_WIDGET(dialog));
  if (result != GTK_RESPONSE_OK)
  {
    return;
  }

  autostart = gtk_toggle_button_get_active(autostart_studio_button);
  notify = gtk_toggle_button_get_active(send_notifications_button);
  shell = gtk_entry_get_text(shell_entry);
  terminal = gtk_entry_get_text(terminal_entry);
  js_delay = gtk_spin_button_get_value(js_delay_spin);

  if (!conf_set_bool(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, autostart) ||
      !conf_set_bool(LADISH_CONF_KEY_DAEMON_NOTIFY, notify) ||
      !conf_set(LADISH_CONF_KEY_DAEMON_SHELL, shell) ||
      !conf_set(LADISH_CONF_KEY_DAEMON_TERMINAL, terminal) ||
      !conf_set_uint(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, js_delay))
  {
    error_message_box(_("Storing settings"));
  }
}
