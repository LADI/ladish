/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the studio list handling code
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

#include "internal.h"
#include "gtk_builder.h"
#include "../proxies/control_proxy.h"

struct studio_list
{
  int count;
  GtkWidget * menu_item;
  GtkWidget * menu;
  void (* item_activate_callback)(GtkWidget * item);
  bool add_sensitive;
};

static struct studio_list g_load_studio_list;
static struct studio_list g_delete_studio_list;

#define studio_list_ptr ((struct studio_list *)context)

static void add_studio_list_menu_entry(void * context, const char * studio_name)
{
  GtkWidget * item;

  item = gtk_menu_item_new_with_label(studio_name);
  //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count); // refcount == 2 because of the label
  gtk_widget_set_sensitive(item, studio_list_ptr->add_sensitive);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(studio_list_ptr->menu), item);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(studio_list_ptr->item_activate_callback), item);
  studio_list_ptr->count++;
}

static void remove_studio_list_menu_entry(GtkWidget * item, gpointer context)
{
  GtkWidget * label;

  label = gtk_bin_get_child(GTK_BIN(item));

  //log_debug("removing studio menu item \"%s\"", gtk_menu_item_get_label(GTK_MENU_ITEM(item));
  // gtk_menu_item_get_label() requries gtk 2.16
  log_debug("removing studio menu item \"%s\"", gtk_label_get_text(GTK_LABEL(label)));

  gtk_container_remove(GTK_CONTAINER(item), label); /* destroy the label and drop the item refcount by one */
  //log_info("refcount == %d", (unsigned int)G_OBJECT(item)->ref_count);
  gtk_container_remove(GTK_CONTAINER(studio_list_ptr->menu), item); /* drop the refcount of item by one and thus destroy it */
  studio_list_ptr->count--;
}

#undef studio_list_ptr

static void menu_studio_list_clear(struct studio_list * studio_list_ptr)
{
  gtk_container_foreach(GTK_CONTAINER(studio_list_ptr->menu), remove_studio_list_menu_entry, studio_list_ptr);
  ASSERT(studio_list_ptr->count == 0);
  studio_list_ptr->count = 0;
}

static void populate_studio_list_menu(GtkMenuItem * menu_item, struct studio_list * studio_list_ptr)
{
  menu_studio_list_clear(studio_list_ptr);
  studio_list_ptr->add_sensitive = true;
  if (!control_proxy_get_studio_list(add_studio_list_menu_entry, studio_list_ptr))
  {
    menu_studio_list_clear(studio_list_ptr);
    studio_list_ptr->add_sensitive = false;
    add_studio_list_menu_entry(studio_list_ptr, "Error obtaining studio list");
  }
  else if (studio_list_ptr->count == 0)
  {
    studio_list_ptr->add_sensitive = false;
    add_studio_list_menu_entry(studio_list_ptr, "Empty studio list");
  }
}

static
void
init_studio_list(
  struct studio_list * studio_list_ptr,
  const char * menu_item,
  const char * menu,
  void (* item_activate_callback)(GtkWidget * item))
{
  studio_list_ptr->count = 0;
  studio_list_ptr->menu_item = get_gtk_builder_widget(menu_item);
  studio_list_ptr->menu = get_gtk_builder_widget(menu);
  studio_list_ptr->item_activate_callback = item_activate_callback;
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(studio_list_ptr->menu_item), studio_list_ptr->menu);
  g_signal_connect(G_OBJECT(studio_list_ptr->menu_item), "activate", G_CALLBACK(populate_studio_list_menu), studio_list_ptr);
}

void init_studio_lists(void)
{
  init_studio_list(&g_load_studio_list, "menu_item_load_studio", "load_studio_menu", on_load_studio);
  init_studio_list(&g_delete_studio_list, "menu_item_delete_studio", "delete_studio_menu", on_delete_studio);
}
