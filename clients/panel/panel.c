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

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <lash/lash.h>
#include <gtk/gtk.h>

#include "panel.h"
#include "project.h"
#include "config.h"

#define WINDOW_TITLE "LASH Control Panel"

void quit_cb(GtkButton * button, void *data);

project_t *
get_project(panel_t * panel, const char *const name)
{
	project_t *project = NULL;
	GtkTreeModel *tree_model = GTK_TREE_MODEL(panel->projects);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(tree_model, &iter))
		do {
			gtk_tree_model_get(tree_model, &iter, PROJECT_PROJECT_COLUMN,
							   &project, -1);

			if (!strcmp(name, project->name))
				return project;

		} while (gtk_tree_model_iter_next(tree_model, &iter));

	fprintf(stderr, "Error: Unable to find project '%s'!", name);
	return NULL;
}

void
error_dialog(panel_t * client, const char *message)
{
	GtkWidget *dialog;
	gint response;

	dialog =
		gtk_message_dialog_new(GTK_WINDOW(client->window),
							   GTK_DIALOG_DESTROY_WITH_PARENT |
							   GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
							   GTK_BUTTONS_OK, "%s", message);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


void
about_cb(GtkButton* button, void* client)
{
	/*panel_t* panel = (panel_t*)client;*/
	GtkWidget *dialog;
	gint response;

	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "LASH Panel");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), PACKAGE_VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
		"Copyright (C) 2005 Dave Robillard");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
		"http://www.nongnu.org/lash");

	

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


void
server_disconnect(panel_t * panel)
{
	error_dialog(panel, "The server disconnected!");

	exit(0);
}

void
event_project_add(panel_t * panel, lash_event_t * event)
{
	project_t *project = NULL;
	const char *name = lash_event_get_string(event);
	GtkTreeIter iter;

	printf("Add project: %s\n", name);

	if (panel->num_projects == 0)
		gtk_label_set_text(GTK_LABEL(panel->no_projects_label), "");
	
	gtk_list_store_append(panel->projects, &iter);

	project = project_create(panel->lash_client, name);

	gtk_list_store_set(panel->projects, &iter,
					   PROJECT_PROJECT_COLUMN, project, -1);

	project->page_number =
		gtk_notebook_append_page(GTK_NOTEBOOK(panel->project_notebook),
								 project->box, project->tab_label);
	
	panel->num_projects++;
}

void
event_project_remove(panel_t * panel, lash_event_t * event)
{
	const char *name = lash_event_get_string(event);
	project_t *project = get_project(panel, name);
	GtkTreeModel *tree_model = GTK_TREE_MODEL(panel->projects);
	GtkTreeIter iter;

	printf("Remove project: %s\n", name);

	if (panel->num_projects == 1)
		gtk_label_set_text(GTK_LABEL(panel->no_projects_label),
			"(No projects open.  Start a LASH client, or load a project from the File menu)");
	
	if (project != NULL)
		if (gtk_tree_model_get_iter_first(tree_model, &iter))
			do {
				gtk_tree_model_get(tree_model, &iter, PROJECT_PROJECT_COLUMN,
								   &project, -1);

				if (!strcmp(name, project->name)) {
					gtk_list_store_remove(panel->projects, &iter);
					gtk_notebook_remove_page(GTK_NOTEBOOK
											 (panel->project_notebook),
											 project->page_number);
					project_destroy(project);
					break;
				}
			} while (gtk_tree_model_iter_next(tree_model, &iter));

	panel->num_projects--;
}

void
event_project_dir(panel_t * panel, lash_event_t * event)
{
	const char *name = lash_event_get_project(event);
	const char *dir = lash_event_get_string(event);
	project_t *project = get_project(panel, name);

	printf("Change project dir: %s = %s\n", name, dir);

	if (project != NULL) {
		project_set_dir(project, dir);
	}
}

void
event_project_name(panel_t * panel, lash_event_t * event)
{
	const char *old_name = lash_event_get_project(event);
	const char *new_name = lash_event_get_string(event);
	project_t *project = get_project(panel, old_name);

	printf("Change project name: %s = %s\n", old_name, new_name);

	if (project != NULL)
		project_set_name(project, new_name);
}

void
event_client_add(panel_t * panel, lash_event_t * event)
{
	const char *project_name = lash_event_get_project(event);
	project_t *project = get_project(panel, project_name);
	char *client_id_str = malloc(37);
	char *search_id_str = NULL;
	uuid_t client_id;
	GtkTreeIter iter;
	GtkTreeModel *tree_model;
	int client_exists = FALSE;
	
	lash_event_get_client_id(event, client_id);
	uuid_unparse(client_id, client_id_str);

	printf("Add client (%s): %s\n", project_name, client_id_str);

	if (client_id != NULL && project != NULL) {
		tree_model = GTK_TREE_MODEL(project->clients);

		if (gtk_tree_model_get_iter_first(tree_model, &iter)) {
			do {
				gtk_tree_model_get(tree_model, &iter, CLIENT_ID_COLUMN, &search_id_str, -1);

				if (!strcmp(search_id_str, client_id_str)) {
					client_exists = TRUE;
					break;
				}
			} while (gtk_tree_model_iter_next(tree_model, &iter));
		}
		if (!client_exists) {
			gtk_list_store_append(project->clients, &iter);
			gtk_list_store_set(project->clients, &iter,
							   CLIENT_ID_COLUMN, client_id_str, -1);
		}
	}
}

void
event_client_name(panel_t * panel, lash_event_t * event)
{
	const char *project_name = lash_event_get_project(event);
	const char *client_name = lash_event_get_string(event);
	project_t *project = get_project(panel, project_name);
	GtkTreeModel *tree_model = NULL;
	GtkTreeIter iter;
	uuid_t client_id;
	char *search_id_str = NULL;
	char *client_id_str = malloc(37);

	lash_event_get_client_id(event, client_id);
	uuid_unparse(client_id, client_id_str);

	printf("Change client name (%s): %s = %s\n", project_name, client_id_str,
		   client_name);

	if (project != NULL) {
		tree_model = GTK_TREE_MODEL(project->clients);

		if (gtk_tree_model_get_iter_first(tree_model, &iter))
			do {
				gtk_tree_model_get(tree_model, &iter, CLIENT_ID_COLUMN,
								   &search_id_str, -1);

				if (!strcmp(search_id_str, client_id_str)) {
					gtk_list_store_set(project->clients, &iter,
									   CLIENT_NAME_COLUMN, client_name, -1);
					break;
				}
			} while (gtk_tree_model_iter_next(tree_model, &iter));
	}
}

void
deal_with_event(panel_t * panel, lash_event_t * event)
{
	switch (lash_event_get_type(event)) {
	case LASH_Project_Add:
		event_project_add(panel, event);
		break;
	case LASH_Project_Remove:
		event_project_remove(panel, event);
		break;
	case LASH_Project_Dir:
		event_project_dir(panel, event);
		break;
	case LASH_Project_Name:
		event_project_name(panel, event);
		break;
	case LASH_Client_Add:
		event_client_add(panel, event);
		break;
	case LASH_Client_Name:
		event_client_name(panel, event);
		break;
	case LASH_Jack_Client_Name:
		break;
	case LASH_Alsa_Client_ID:
		break;
	case LASH_Percentage:
		break;
	default:
		break;
	}
}

gboolean
idle_cb(gpointer data)
{
	panel_t *panel;
	lash_event_t *event;

	/*lash_config_t * config; */

	panel = (panel_t *) data;

	while ((event = lash_get_event(panel->lash_client))) {
		deal_with_event(panel, event);
	}

	/*while ((config = lash_get_config(panel->lash_client)) ) {
	 * add_config(panel, config);
	 * } */

	return TRUE;
}

void
quit_cb(GtkButton * button, void *data)
{
	gtk_main_quit();
}

void
open_cb(GtkButton * button, void *data)
{
	panel_t *panel = (panel_t *) data;
	int response = GTK_RESPONSE_NONE;
	char *filename = NULL;
	lash_event_t *event = NULL;

	GtkWidget *open_dialog =
		gtk_file_chooser_dialog_new("Open Project", GTK_WINDOW(panel->window),
									GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	response = gtk_dialog_run(GTK_DIALOG(open_dialog));

	if (response == GTK_RESPONSE_OK) {
		filename =
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_dialog));
		event = lash_event_new_with_type(LASH_Project_Add);
		lash_event_set_string(event, filename);
		lash_send_event(panel->lash_client, event);
	}

	gtk_widget_destroy(open_dialog);
}

panel_t *
panel_create(lash_client_t * lash_client)
{
	panel_t *panel = NULL;
	GtkWidget *main_box = NULL;

	GtkWidget *menu_bar = NULL;
	GtkWidget *file_menu = NULL;
	GtkWidget *file_menuitem = NULL;
	GtkWidget *help_menu = NULL;
	GtkWidget *help_menuitem = NULL;
	GtkWidget *open_menu_item = NULL;
	GtkWidget *menu_separator = NULL;
	GtkWidget *quit_menu_item = NULL;
	GtkWidget *about_menu_item = NULL;

	guint status_context;

	panel = lash_malloc(sizeof(panel_t));
	panel->lash_client = lash_client;

	panel->projects = gtk_list_store_new(PROJECT_NUM_COLUMNS, G_TYPE_POINTER);
	panel->num_projects = 0;

	panel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(panel->window), WINDOW_TITLE);
	gtk_window_set_default_size(GTK_WINDOW(panel->window), 450, 282);
	g_signal_connect(G_OBJECT(panel->window), "delete_event",
					 G_CALLBACK(quit_cb), NULL);

	main_box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_box);
	gtk_container_add(GTK_CONTAINER(panel->window), main_box);

	/*
	 * menu bar
	 */
	file_menu = gtk_menu_new();

	open_menu_item = gtk_image_menu_item_new_with_label("Open Project...");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(open_menu_item),
								  gtk_image_new_from_stock(GTK_STOCK_OPEN,
														   GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_menu_item);
	g_signal_connect(G_OBJECT(open_menu_item), "activate",
					 G_CALLBACK(open_cb), panel);
	gtk_widget_show(open_menu_item);
	
	menu_separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), menu_separator);
	gtk_widget_show(menu_separator);

	quit_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_menu_item);
	g_signal_connect(G_OBJECT(quit_menu_item), "activate",
					 G_CALLBACK(quit_cb), panel);
	gtk_widget_show(quit_menu_item);

	file_menuitem = gtk_menu_item_new_with_label("File");
	gtk_widget_show(file_menuitem);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menuitem), file_menu);

	help_menu = gtk_menu_new();
	help_menuitem = gtk_menu_item_new_with_label("Help");
	gtk_widget_show(help_menuitem);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menuitem), help_menu);

	about_menu_item = gtk_image_menu_item_new_with_label("About...");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(about_menu_item),
								  gtk_image_new_from_stock(GTK_STOCK_ABOUT,
														   GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_menu_item);
	g_signal_connect(G_OBJECT(about_menu_item), "activate",
					 G_CALLBACK(about_cb), panel);
	
	gtk_widget_show(about_menu_item);
	
	menu_bar = gtk_menu_bar_new();
	gtk_widget_show(menu_bar);
	gtk_box_pack_start(GTK_BOX(main_box), menu_bar, FALSE, TRUE, 0);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_menuitem);

	/*
	 * projects notebook
	 */
	panel->project_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(panel->project_notebook),
							 GTK_POS_TOP);
	gtk_widget_show(panel->project_notebook);
	gtk_box_pack_start(GTK_BOX(main_box), panel->project_notebook, TRUE, TRUE, 0);

	/*
	 * "no project" label (shown instead of a notebook tab when no projects are present)
	 */
	panel->no_projects_label = gtk_label_new("No projects open.  Start a LASH client, or load a project from the File menu.");
	gtk_widget_show(panel->no_projects_label);
	gtk_box_pack_start(GTK_BOX(main_box), panel->no_projects_label, FALSE, TRUE, 10);
	
	/*
	 * status bar
	 */
	panel->status_bar = gtk_statusbar_new();
	gtk_widget_show(panel->status_bar);
	status_context =
		gtk_statusbar_get_context_id(GTK_STATUSBAR(panel->status_bar),
									 "connected");
	gtk_statusbar_push(GTK_STATUSBAR(panel->status_bar), status_context,
					   g_strdup_printf("Connected to server %s",
									   lash_get_server_name(lash_client)));
	gtk_box_pack_start(GTK_BOX(main_box), panel->status_bar, FALSE, TRUE, 0);

	gtk_timeout_add(1000, idle_cb, panel);
	gtk_widget_show(panel->window);

	return panel;
}
