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

#include "project.h"
#include <malloc.h>
#include <lash/lash.h>
#include <assert.h>

/* Button callbacks */

void
save_cb(GtkButton* button, void* data)
{
	project_t*    project = (project_t*)data;
	lash_event_t* event   = lash_event_new_with_type(LASH_Save);
	
	lash_event_set_project(event, project->name);
	lash_send_event(project->lash_client, event);

	printf("Told server to save project %s\n", project->name);
}


void
close_cb(GtkButton * button, void * data)
{
	project_t*    project      = (project_t*)data;
	lash_event_t* event        = lash_event_new_with_type(LASH_Project_Remove);
	
	lash_event_set_project(event, project->name);
	lash_send_event(project->lash_client, event);

	printf ("Told server to close project %s\n", project->name);
}


void
set_dir_cb(GtkButton * button, void* data)
{
	project_t*    project    = (project_t*)data;
	int           response = GTK_RESPONSE_NONE;
	char*         filename = NULL;
	lash_event_t* event    = NULL;

	GtkWidget* open_dialog = gtk_file_chooser_dialog_new(
		"Set Project Directory", NULL,
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	response = gtk_dialog_run(GTK_DIALOG(open_dialog));

	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_dialog));
		project_set_dir(project, filename);
		event = lash_event_new_with_type(LASH_Project_Dir);
		lash_event_set_project(event, project->name);
		lash_event_set_string(event, filename);
		lash_send_event(project->lash_client, event);
	}

	gtk_widget_destroy(open_dialog);
}


/* Project */


project_t*
project_create(lash_client_t* lash_client, const char* const name)
{
	project_t* project = (project_t*)malloc(sizeof(project_t));

	project->lash_client = lash_client;
	project->name = NULL;
	project->dir = NULL;
	project->page_number = -1;

	project->box = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(project->box);
	gtk_container_set_border_width(GTK_CONTAINER(project->box), 8);
	
	project->tab_label = gtk_label_new("Unnamed project");
	gtk_widget_show(project->tab_label);

	/* Properties header */
	project->properties_label = gtk_label_new("<span weight=\"bold\">Properties</span>");
	gtk_widget_show(project->properties_label);
	gtk_misc_set_alignment(GTK_MISC(project->properties_label), 0.0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(project->properties_label), TRUE);
	gtk_box_pack_start(GTK_BOX(project->box), project->properties_label, FALSE, FALSE, 0);
	
	/* Name label */
	project->name_box = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(project->name_box);
	gtk_box_pack_start(GTK_BOX(project->box), project->name_box, FALSE, TRUE, 0);
	
	project->name_label = gtk_label_new("    Name: ");
	gtk_widget_show(project->name_label);
	gtk_box_pack_start(GTK_BOX(project->name_box), project->name_label, FALSE, FALSE, 0);

	project->name_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(project->name_entry), FALSE);
	gtk_widget_show(project->name_entry);
	gtk_box_pack_start(GTK_BOX(project->name_box), project->name_entry, TRUE, TRUE, 0);
	
	/* Directory label */
	project->dir_box = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(project->dir_box);
	gtk_box_pack_start(GTK_BOX(project->box), project->dir_box, FALSE, TRUE, 0);
	
	project->dir_label = gtk_label_new("    Directory: ");
	gtk_widget_show(project->dir_label);
	gtk_box_pack_start(GTK_BOX(project->dir_box), project->dir_label, FALSE, FALSE, 0);

	project->dir_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(project->dir_entry), FALSE);
	gtk_widget_show(project->dir_entry);
	gtk_box_pack_start(GTK_BOX(project->dir_box), project->dir_entry, TRUE, TRUE, 0);
	
	project->set_dir_button = gtk_button_new_with_label("Select...");
	gtk_widget_show(project->set_dir_button);
	g_signal_connect(G_OBJECT(project->set_dir_button), "clicked", G_CALLBACK(set_dir_cb), project);
	gtk_box_pack_start(GTK_BOX(project->dir_box), project->set_dir_button, FALSE, TRUE, 6);
	
	/* Clients header */
	project->clients_label = gtk_label_new("<span weight=\"bold\">Clients</span>");
	gtk_widget_show(project->clients_label);
	gtk_misc_set_alignment(GTK_MISC(project->clients_label), 0.0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(project->clients_label), TRUE);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_label, FALSE, FALSE, 0);
	
	project->clients_align_box = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(project->clients_align_box);
	project->clients_align_label = gtk_label_new("    ");
	gtk_widget_show(project->clients_align_label);
	gtk_box_pack_start(GTK_BOX(project->clients_align_box), project->clients_align_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_align_box, TRUE, TRUE, 0);

	project->clients_list_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(project->clients_list_scroll);
	gtk_box_pack_start(GTK_BOX(project->clients_align_box), project->clients_list_scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(project->clients_list_scroll),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Client list */
	project->clients = gtk_list_store_new(CLIENT_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	project->clients_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(project->clients));
	gtk_widget_show(project->clients_list);
	gtk_container_add(GTK_CONTAINER(project->clients_list_scroll), project->clients_list);

	/* Name column */
	project->clients_renderer = gtk_cell_renderer_text_new();
	project->name_column = gtk_tree_view_column_new_with_attributes("Name", project->clients_renderer,
		"text", CLIENT_NAME_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(project->clients_list), project->name_column);
	
	/* ID column */
	project->id_column = gtk_tree_view_column_new_with_attributes("ID", project->clients_renderer,
		"text", CLIENT_ID_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(project->clients_list), project->id_column);
	
	/* Separator */
	project->buttons_separator = gtk_hseparator_new();
	gtk_widget_show(project->buttons_separator);
	gtk_box_pack_start(GTK_BOX(project->box), project->buttons_separator, FALSE, TRUE, 6);
	
	/* Buttons */
	project->clients_button_box = gtk_hbutton_box_new();
	gtk_widget_show(project->clients_button_box);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(project->clients_button_box), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_button_box, FALSE, TRUE, 0);

	/* Close button */
	project->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_show(project->close_button);
	g_signal_connect(G_OBJECT(project->close_button), "clicked", G_CALLBACK(close_cb), project);
	gtk_box_pack_start(GTK_BOX(project->clients_button_box), project->close_button, FALSE, TRUE, 6);
	
	/* Save button */
	project->save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_widget_show(project->save_button);
	g_signal_connect(G_OBJECT(project->save_button), "clicked", G_CALLBACK(save_cb), project);
	gtk_box_pack_start(GTK_BOX(project->clients_button_box), project->save_button, FALSE, TRUE, 6);
	

	
	/* Set name (field and tab label) */
	project_set_name(project, name);

	return project;
}


void
project_destroy(project_t* project)
{
	assert(project != NULL);

	free(project->name);
	
	/* FIXME: getting warnings/segfault here
	gtk_widget_destroy(project->box);
	gtk_widget_destroy(project->tab_label);
	
	gtk_widget_destroy(project->clients_label);
	gtk_widget_destroy(project->clients_align_box);
	gtk_widget_destroy(project->clients_align_label);
	gtk_widget_destroy(project->clients_list);
	gtk_widget_destroy(project->clients_list_scroll);
	gtk_object_destroy(GTK_OBJECT(project->clients_renderer));
	gtk_object_destroy(GTK_OBJECT(project->name_column));
	gtk_object_destroy(GTK_OBJECT(project->id_column));
	gtk_widget_destroy(project->clients_button_box);
	gtk_widget_destroy(project->save_button);
	gtk_widget_destroy(project->close_button);*/

	free(project);
}


void
project_set_name(project_t* project, const char* const name)
{
	assert(project != NULL);
	free(project->name);
	project->name = malloc(sizeof(char) * (strlen(name)+1));
	strncpy(project->name, name, strlen(name)+1);
	gtk_label_set_text(GTK_LABEL(project->tab_label), name);
	gtk_entry_set_text(GTK_ENTRY(project->name_entry), name);
}


void
project_set_dir(project_t* project, const char* const dir)
{
	assert(project != NULL);
	free(project->dir);
	project->dir = malloc(sizeof(char) * (strlen(dir)+1));
	strncpy(project->dir, dir, strlen(dir)+1);
	gtk_entry_set_text(GTK_ENTRY(project->dir_entry), dir);
}

