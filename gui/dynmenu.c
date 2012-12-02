/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains dynamic menu related code
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

#include "dynmenu.h"
#include "gtk_builder.h"
#include "../common/catdup.h"

struct ladish_dynmenu
{
  int count;
  GtkWidget * menu_item;
  GtkWidget * menu;
  bool
  (* fill_callback)(
    void
    (* callback)(
      void * context,
      const char * name,
      void * data,
      ladish_dynmenu_item_activate_callback item_activate_callback,
      void (* data_free)()),
    void * context);
  ladish_dynmenu_item_activate_callback item_activate_callback;
  bool add_sensitive;
  gulong activate_signal_id;
  char * description;
};

struct ladish_dynmenu_item_data
{
  GtkWidget * item;
  void * data;
  void (* data_free)();
  ladish_dynmenu_item_activate_callback item_activate_callback;
};

void on_activate_item(GtkMenuItem * UNUSED(item), struct ladish_dynmenu_item_data * data_ptr)
{
  //log_info("on_activate_item('%s')", gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(data_ptr->item)))));

  data_ptr->item_activate_callback(
    gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(data_ptr->item)))),
    data_ptr->data);
}

#define data_ptr ((struct ladish_dynmenu_item_data *)data)

void free_item_data(gpointer data, GClosure * UNUSED(closure))
{
  //log_info("data_ptr %p deallocate", data_ptr);

  if (data_ptr->data != NULL)
  {
    if (data_ptr->data_free != NULL)
    {
      data_ptr->data_free(data_ptr->data);
    }
    else
    {
      free(data_ptr->data);
    }
  }

  free(data_ptr);
}

#undef data_ptr
#define dynmenu_ptr ((struct ladish_dynmenu *)context)

static
void
ladish_dynmenu_add_entry(
  void * context,
  const char * name,
  void * data,
  ladish_dynmenu_item_activate_callback item_activate_callback,
  void (* data_free)())
{
  struct ladish_dynmenu_item_data * data_ptr;
  GtkWidget * item;

  if (name == NULL)
  {
    /* add separator */
    ASSERT(data == NULL);
    ASSERT(item_activate_callback == NULL);
    ASSERT(data_free == NULL);
    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(dynmenu_ptr->menu), item);
  }
  else
  {
    data_ptr = malloc(sizeof(struct ladish_dynmenu_item_data));
    if (data_ptr == NULL)
    {
      log_error("Allocation of memory for dynmenu item data struct failed.");
      return;
    }

    //log_info("data_ptr %p allocated", data_ptr);
    data_ptr->data = data;
    data_ptr->data_free = data_free;
    data_ptr->item_activate_callback = item_activate_callback != NULL ? item_activate_callback : dynmenu_ptr->item_activate_callback;

    data_ptr->item = gtk_menu_item_new_with_label(name);
    //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count); // refcount == 2 because of the label
    gtk_widget_set_sensitive(data_ptr->item, dynmenu_ptr->add_sensitive);
    gtk_widget_show(data_ptr->item);
    gtk_menu_shell_append(GTK_MENU_SHELL(dynmenu_ptr->menu), data_ptr->item);
    g_signal_connect_data(
      G_OBJECT(data_ptr->item),
      "activate",
      G_CALLBACK(on_activate_item),
      data_ptr,
      free_item_data,
      (GConnectFlags)0);
  }

  dynmenu_ptr->count++;
}

static void remove_dynmenu_menu_entry(GtkWidget * item, gpointer context)
{
  GtkWidget * label;

  label = gtk_bin_get_child(GTK_BIN(item));

  //log_debug("removing dynmenu item \"%s\"", gtk_menu_item_get_label(GTK_MENU_ITEM(item));
  // gtk_menu_item_get_label() requries gtk 2.16
  log_debug("removing dynmenu item \"%s\"", gtk_label_get_text(GTK_LABEL(label)));

  gtk_container_remove(GTK_CONTAINER(item), label); /* destroy the label and drop the item refcount by one */
  //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count);
  gtk_container_remove(GTK_CONTAINER(dynmenu_ptr->menu), item); /* drop the refcount of item by one and thus destroy it */
  dynmenu_ptr->count--;
}

#undef dynmenu_ptr

static void menu_dynmenu_clear(struct ladish_dynmenu * dynmenu_ptr)
{
  gtk_container_foreach(GTK_CONTAINER(dynmenu_ptr->menu), remove_dynmenu_menu_entry, dynmenu_ptr);
  ASSERT(dynmenu_ptr->count == 0);
  dynmenu_ptr->count = 0;
}

static void populate_dynmenu_menu(GtkMenuItem * menu_item, struct ladish_dynmenu * dynmenu_ptr)
{
  const char * prefix;
  char * text;

  if (!gtk_widget_get_sensitive(GTK_WIDGET(menu_item)))
  {
    return;
  }

  menu_dynmenu_clear(dynmenu_ptr);
  dynmenu_ptr->add_sensitive = true;
  if (!dynmenu_ptr->fill_callback(ladish_dynmenu_add_entry, dynmenu_ptr))
  {
    menu_dynmenu_clear(dynmenu_ptr);
    prefix = _("Error filling ");
  }
  else if (dynmenu_ptr->count == 0)
  {
    prefix = _("Empty ");
  }
  else
  {
    return;
  }

  text = catdup(prefix, dynmenu_ptr->description);

  dynmenu_ptr->add_sensitive = false;
  ladish_dynmenu_add_entry(dynmenu_ptr, text != NULL ? text : prefix, NULL, NULL, NULL);

  free(text);                   /* free(NULL) is safe */
}

bool
ladish_dynmenu_create(
  const char * menu_item,
  const char * menu,
  bool
  (* fill_callback)(
    void
    (* callback)(
      void * context,
      const char * name,
      void * data,
      ladish_dynmenu_item_activate_callback item_activate_callback,
      void (* data_free)()),
    void * context),
  const char * description,
  ladish_dynmenu_item_activate_callback item_activate_callback,
  ladish_dynmenu_handle * dynmenu_handle_ptr)
{
  struct ladish_dynmenu * dynmenu_ptr;

  dynmenu_ptr = malloc(sizeof(struct ladish_dynmenu));
  if (dynmenu_ptr == NULL)
  {
    log_error("Allocation of ladish_dynmenu struct failed");
    return false;
  }

  dynmenu_ptr->description = strdup(description);
  if (dynmenu_ptr->description == NULL)
  {
    log_error("strdup('%s') failed for dynmenu description string", description);
    free(dynmenu_ptr);
    return false;
  }

  dynmenu_ptr->count = 0;
  dynmenu_ptr->menu_item = get_gtk_builder_widget(menu_item);
  dynmenu_ptr->menu = get_gtk_builder_widget(menu);
  dynmenu_ptr->fill_callback = fill_callback;
  dynmenu_ptr->item_activate_callback = item_activate_callback;
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(dynmenu_ptr->menu_item), dynmenu_ptr->menu);
  dynmenu_ptr->activate_signal_id = g_signal_connect(G_OBJECT(dynmenu_ptr->menu_item), "activate", G_CALLBACK(populate_dynmenu_menu), dynmenu_ptr);

  *dynmenu_handle_ptr = (ladish_dynmenu_handle)dynmenu_ptr;
  return true;
}

#define dynmenu_ptr ((struct ladish_dynmenu *)dynmenu_handle)

void
ladish_dynmenu_fill_external(
  ladish_dynmenu_handle dynmenu_handle,
  GtkMenu * menu)
{
  GtkWidget * menu_backup;
  int count_backup;

  menu_backup = dynmenu_ptr->menu;
  count_backup = dynmenu_ptr->count;

  dynmenu_ptr->menu = GTK_WIDGET(menu);
  dynmenu_ptr->add_sensitive = true;

  dynmenu_ptr->fill_callback(ladish_dynmenu_add_entry, dynmenu_ptr);

  dynmenu_ptr->menu = menu_backup;
  dynmenu_ptr->count = count_backup;
}

void
ladish_dynmenu_destroy(
  ladish_dynmenu_handle dynmenu_handle)
{
  if (g_signal_handler_is_connected(G_OBJECT(dynmenu_ptr->menu_item), dynmenu_ptr->activate_signal_id))
  {
    g_signal_handler_disconnect(G_OBJECT(dynmenu_ptr->menu_item), dynmenu_ptr->activate_signal_id);
  }

  free(dynmenu_ptr->description);
  free(dynmenu_ptr);
}

#undef dynmenu_ptr
