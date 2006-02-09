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
#include <unistd.h>

/* Button callbacks */

void
save_cb(GtkButton * button, void *data)
{
	project_t *project = (project_t *) data;
	lash_event_t *event = lash_event_new_with_type(LASH_Save);

	lash_event_set_project(event, project->name);
	lash_send_event(project->lash_client, event);

	printf("Told server to save project %s\n", project->name);
}

void
close_cb(GtkButton * button, void *data)
{
	project_t *project = (project_t *) data;
	lash_event_t *event = lash_event_new_with_type(LASH_Project_Remove);

	lash_event_set_project(event, project->name);
	lash_send_event(project->lash_client, event);

	printf("Told server to close project %s\n", project->name);
}

void
set_dir_cb(GtkButton * button, void *data)
{
	project_t *project = (project_t *) data;
	int response = GTK_RESPONSE_NONE;
	char *filename = NULL;
	lash_event_t *event = NULL;

	GtkWidget *open_dialog =
		gtk_file_chooser_dialog_new("Set Project Directory", NULL,
									GTK_FILE_CHOOSER_ACTION_SAVE,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	response = gtk_dialog_run(GTK_DIALOG(open_dialog));

	if (response == GTK_RESPONSE_OK) {
		filename =
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_dialog));
		project_set_dir(project, filename);
		event = lash_event_new_with_type(LASH_Project_Dir);
		lash_event_set_project(event, project->name);
		lash_event_set_string(event, filename);
		lash_send_event(project->lash_client, event);

		printf("Told server to set project directory %s\n", filename);
	}

	gtk_widget_destroy(open_dialog);
}

void
set_name_cb(GtkEntry * entry, void *data)
{
	project_t *project = (project_t *) data;
	lash_event_t *event = NULL;
	const char *new_name = gtk_entry_get_text(GTK_ENTRY(project->name_entry));

	printf("Name changed: %s\n", new_name);

	event = lash_event_new_with_type(LASH_Project_Name);
	lash_event_set_project(event, project->name);
	lash_event_set_string(event, new_name);
	lash_send_event(project->lash_client, event);
}

/* Project */

project_t *
project_create(lash_client_t * lash_client, const char *const name)
{
	project_t *project = (project_t *) malloc(sizeof(project_t));

	project->lash_client = lash_client;
	project->name = NULL;
	project->dir = NULL;
	project->page_number = -1;

	project->box = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(project->box);
	gtk_container_set_border_width(GTK_CONTAINER(project->box), 8);

	project->tab_label = gtk_label_new("Unnamed project");
	gtk_widget_show(project->tab_label);

	/* Clients header */
	project->clients_label =
		gtk_label_new("<span weight=\"bold\">Clients</span>");
	gtk_widget_show(project->clients_label);
	gtk_misc_set_alignment(GTK_MISC(project->clients_label), 0.0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(project->clients_label), TRUE);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_label, FALSE,
					   FALSE, 0);

	project->clients_align_box = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(project->clients_align_box);
	project->clients_align_label = gtk_label_new("    ");
	gtk_widget_show(project->clients_align_label);
	gtk_box_pack_start(GTK_BOX(project->clients_align_box),
					   project->clients_align_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_align_box,
					   TRUE, TRUE, 0);

	project->clients_list_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(project->clients_list_scroll);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(project->clients_list_scroll),
		GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(project->clients_align_box),
					   project->clients_list_scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
								   (project->clients_list_scroll),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	/* Properties header */
	project->properties_label =
		gtk_label_new("<span weight=\"bold\">Properties</span>");
	gtk_widget_show(project->properties_label);
	gtk_misc_set_alignment(GTK_MISC(project->properties_label), 0.0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(project->properties_label), TRUE);
	gtk_box_pack_start(GTK_BOX(project->box), project->properties_label,
					   FALSE, FALSE, 0);

	/* Properties table */
	project->properties_table = gtk_table_new(2, 3, FALSE);
	gtk_widget_show(project->properties_table);
	gtk_box_pack_start(GTK_BOX(project->box), project->properties_table,
					   FALSE, TRUE, 0);

	/* Name label */
	project->name_label = gtk_label_new("    Name: ");
	gtk_misc_set_alignment(GTK_MISC(project->name_label), 0.0, 0.5);
	gtk_widget_show(project->name_label);
	gtk_table_attach(GTK_TABLE(project->properties_table),
					 project->name_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 2, 2);

	project->name_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(project->name_entry), TRUE);
	gtk_widget_show(project->name_entry);
	g_signal_connect(G_OBJECT(project->name_entry), "activate",
					 G_CALLBACK(set_name_cb), project);
	gtk_table_attach(GTK_TABLE(project->properties_table),
					 project->name_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
					 0, 2, 2);

	project->set_name_button = gtk_button_new_with_label("Apply");
	gtk_widget_show(project->set_name_button);
	g_signal_connect(G_OBJECT(project->set_name_button), "clicked",
					 G_CALLBACK(set_name_cb), project);
	gtk_table_attach(GTK_TABLE(project->properties_table),
					 project->set_name_button, 2, 3, 0, 1, GTK_FILL, GTK_FILL,
					 2, 2);

	/* Directory label */
	project->dir_label = gtk_label_new("    Directory: ");
	gtk_misc_set_alignment(GTK_MISC(project->dir_label), 0.0, 0.5);
	gtk_widget_show(project->dir_label);
	gtk_table_attach(GTK_TABLE(project->properties_table), project->dir_label,
					 0, 1, 1, 2, GTK_FILL, GTK_FILL, 2, 2);

	project->dir_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(project->dir_entry), FALSE);
	gtk_widget_show(project->dir_entry);
	gtk_table_attach(GTK_TABLE(project->properties_table), project->dir_entry,
					 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 2, 2);

	project->set_dir_button = gtk_button_new_with_label("Move...");
	gtk_widget_show(project->set_dir_button);
	g_signal_connect(G_OBJECT(project->set_dir_button), "clicked",
					 G_CALLBACK(set_dir_cb), project);
	gtk_table_attach(GTK_TABLE(project->properties_table),
					 project->set_dir_button, 2, 3, 1, 2, GTK_FILL, GTK_FILL,
					 2, 2);

	/* Buttons */
	project->clients_button_box = gtk_hbutton_box_new();
	gtk_widget_show(project->clients_button_box);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(project->clients_button_box),
							  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(project->clients_button_box),
							   4);
	gtk_box_pack_start(GTK_BOX(project->box), project->clients_button_box,
					   FALSE, TRUE, 4);

	/* Close button */
	project->close_button = gtk_button_new_with_label("Close Project");
	gtk_button_set_image(GTK_BUTTON(project->close_button),
		gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON));
	gtk_widget_show(project->close_button);
	g_signal_connect(G_OBJECT(project->close_button), "clicked",
					 G_CALLBACK(close_cb), project);
	gtk_box_pack_start(GTK_BOX(project->clients_button_box),
					   project->close_button, FALSE, TRUE, 6);

	/* Save button */
	project->save_button = gtk_button_new_with_label("Save Project");
	gtk_button_set_image(GTK_BUTTON(project->save_button),
		gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON));
	gtk_widget_show(project->save_button);
	g_signal_connect(G_OBJECT(project->save_button), "clicked",
					 G_CALLBACK(save_cb), project);
	gtk_box_pack_start(GTK_BOX(project->clients_button_box),
					   project->save_button, FALSE, TRUE, 6);

	/* Client list */
	project->clients = gtk_list_store_new(CLIENT_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	project->clients_list =
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(project->clients));
	gtk_widget_show(project->clients_list);
	gtk_container_add(GTK_CONTAINER(project->clients_list_scroll),
					  project->clients_list);

	/* Name column */
	project->clients_renderer = gtk_cell_renderer_text_new();
	project->name_column =
		gtk_tree_view_column_new_with_attributes("Name",
												 project->clients_renderer,
												 "text", CLIENT_NAME_COLUMN,
												 NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(project->clients_list),
								project->name_column);

	/* ID column */
	project->id_column =
		gtk_tree_view_column_new_with_attributes("ID",
												 project->clients_renderer,
												 "text", CLIENT_ID_COLUMN,
												 NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(project->clients_list),
								project->id_column);

	/* Set name (field and tab label) */
	project_set_name(project, name);

	return project;
}

void
project_destroy(project_t * project)
{
	assert(project != NULL);

	free(project->name);

	/* FIXME: getting warnings/segfault here
	 * gtk_widget_destroy(project->box);
	 * gtk_widget_destroy(project->tab_label);
	 * 
	 * gtk_widget_destroy(project->clients_label);
	 * gtk_widget_destroy(project->clients_align_box);
	 * gtk_widget_destroy(project->clients_align_label);
	 * gtk_widget_destroy(project->clients_list);
	 * gtk_widget_destroy(project->clients_list_scroll);
	 * gtk_object_destroy(GTK_OBJECT(project->clients_renderer));
	 * gtk_object_destroy(GTK_OBJECT(project->name_column));
	 * gtk_object_destroy(GTK_OBJECT(project->id_column));
	 * gtk_widget_destroy(project->clients_button_box);
	 * gtk_widget_destroy(project->save_button);
	 * gtk_widget_destroy(project->close_button); */

	free(project);
}

void
project_set_name(project_t * project, const char *const name)
{
	assert(project != NULL);
	assert(name != NULL);
	free(project->name);
	project->name = calloc(strlen(name) + 1, sizeof(char));
	strncpy(project->name, name, strlen(name) + 1);
	gtk_label_set_text(GTK_LABEL(project->tab_label), project->name);
	gtk_entry_set_text(GTK_ENTRY(project->name_entry), project->name);
}

void
project_set_dir(project_t * project, const char *const dir)
{
	assert(project != NULL);
	free(project->dir);
	project->dir = calloc(strlen(dir) + 1, sizeof(char));
	strncpy(project->dir, dir, strlen(dir) + 1);
	gtk_entry_set_text(GTK_ENTRY(project->dir_entry), project->dir);
}
