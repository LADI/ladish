/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that implements ask dialog
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

#include <stdarg.h>
#include "ask_dialog.h"
#include "gtk_builder.h"

bool
ask_dialog(
  bool * result_ptr,            /* true - yes, false - no */
  const char * text,
  const char * secondary_text_format,
  ...)
{
  GtkResponseType response;
  GtkWidget * dialog;
  gchar * msg;
  va_list ap;

  dialog = get_gtk_builder_widget("ask_dialog");

  va_start(ap, secondary_text_format);
  msg = g_markup_vprintf_escaped(secondary_text_format, ap);
  va_end(ap);
  if (msg == NULL)
  {
    log_error("g_markup_vprintf_escaped() failed.");
    return false;
  }
  gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), _("%s"), msg);
  g_free(msg);

  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), text);
  gtk_widget_show(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
  switch (response)
  {
  case GTK_RESPONSE_YES:
    *result_ptr = true;
    return true;
  case GTK_RESPONSE_NO:
    *result_ptr = false;
    return true;
  default:                      /* ignore warning */
    break;
  }

  return false;
}
