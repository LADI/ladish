/*
 *   LASH 
 *    
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <lash/lash.h>
#include <gtk/gtk.h>

#include "gtk_client.h"

#define WINDOW_TITLE "LASH GTK Test Client"

void quit_cb(GtkButton * button, void *data);

void
error_dialog(gtk_client_t * client, const char *message)
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
save_file(gtk_client_t * gtk_client, const char *fqn)
{
	FILE *file;
	GtkTextIter start_iter, end_iter;
	gchar *text;
	int err;

	gtk_text_buffer_get_start_iter(gtk_client->file_data, &start_iter);
	gtk_text_buffer_get_end_iter(gtk_client->file_data, &end_iter);
	text =
		gtk_text_buffer_get_text(gtk_client->file_data, &start_iter,
								 &end_iter, TRUE);

	file = fopen(fqn, "w");
	if (!file) {
		char *message;

		message =
			g_strdup_printf("Error opening file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		return;
	}

	err = fprintf(file, "%s", text);
	if (err < 0) {
		char *message;

		message =
			g_strdup_printf("Error writing to file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		fclose(file);
		return;
	}

	err = fclose(file);
	if (err == -1) {
		char *message;

		message =
			g_strdup_printf("Error closing file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		return;
	}
}

void
open_file(gtk_client_t * gtk_client, const char *fqn)
{
	FILE *file;
	char *buf;
	size_t buf_size = 32;
	int ch;
	int read = 0;
	int err;

	file = fopen(fqn, "r");
	if (!file) {
		char *message;

		message =
			g_strdup_printf("Error opening file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		return;
	}

	buf = lash_malloc(buf_size);

	while ((ch = fgetc(file)) != EOF) {
		if (read == buf_size) {
			buf_size *= 2;
			buf = lash_realloc(buf, buf_size);
		}

		buf[read++] = ch;
	}

	if (ferror(file)) {
		char *message;

		message =
			g_strdup_printf("Error reading from file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		free(buf);
		fclose(file);
		return;
	}

	err = fclose(file);
	if (err == -1) {
		char *message;

		message =
			g_strdup_printf("Error closing file '%s':\n\n%s", fqn,
							strerror(errno));
		error_dialog(gtk_client, message);
		free(message);
		return;
	}

	gtk_text_buffer_set_text(gtk_client->file_data, buf, read);
	free(buf);
}

void
save_configs(gtk_client_t * gtk_client)
{
	lash_config_t *config;
	lash_event_t *event;
	GtkTreeIter iter;
	char *key, *value;
	GtkTreeModel *tree_model;

	tree_model = GTK_TREE_MODEL(gtk_client->configs);
	if (gtk_tree_model_get_iter_first(tree_model, &iter))
		do {
			config = lash_config_new();

			/* extract the data */
			gtk_tree_model_get(tree_model, &iter,
							   KEY_COLUMN, &key, VALUE_COLUMN, &value, -1);

			lash_config_set_key(config, key);
			lash_config_set_value_string(config, value);

			free(key);
			free(value);

			lash_send_config(gtk_client->lash_client, config);
		}
		while (gtk_tree_model_iter_next(tree_model, &iter));

	event = lash_event_new_with_type(LASH_Save_Data_Set);
	lash_send_event(gtk_client->lash_client, event);
}

void
clear_configs(gtk_client_t * gtk_client)
{
	GtkTreeIter iter;
	GtkTreeModel *tree_model;

	tree_model = GTK_TREE_MODEL(gtk_client->configs);

	if (gtk_tree_model_get_iter_first(tree_model, &iter))
		do {
			gtk_list_store_remove(gtk_client->configs, &iter);
		}
		while (gtk_tree_model_iter_has_child(tree_model, &iter));
}

void
add_config(gtk_client_t * gtk_client, lash_config_t * config)
{
	GtkTreeIter iter;

	gtk_list_store_append(gtk_client->configs, &iter);

	gtk_list_store_set(gtk_client->configs, &iter,
					   KEY_COLUMN, lash_config_get_key(config),
					   VALUE_COLUMN, lash_config_get_value_string(config),
					   -1);

	lash_config_destroy(config);
}

void
server_disconnect(gtk_client_t * gtk_client)
{
	error_dialog(gtk_client, "The server disconnected!");

	exit(0);
}

void
deal_with_event(gtk_client_t * gtk_client, lash_event_t * event)
{
	switch (lash_event_get_type(event)) {
	case LASH_Save_File:
		save_file(gtk_client,
				  lash_get_fqn(lash_event_get_string(event), filename));
		lash_send_event(gtk_client->lash_client, event);
		break;
	case LASH_Restore_File:
		open_file(gtk_client,
				  lash_get_fqn(lash_event_get_string(event), filename));
		lash_send_event(gtk_client->lash_client, event);
		break;
	case LASH_Save_Data_Set:
		save_configs(gtk_client);
		lash_send_event(gtk_client->lash_client, event);
		break;
	case LASH_Restore_Data_Set:
		clear_configs(gtk_client);
		lash_send_event(gtk_client->lash_client, event);
		break;
	case LASH_Quit:
		quit_cb(NULL, NULL);
		lash_event_destroy(event);
		break;
	case LASH_Server_Lost:
		server_disconnect(gtk_client);
		break;
	default:
		break;
	}
}

gboolean
idle_cb(gpointer data)
{
	gtk_client_t *gtk_client;
	lash_event_t *event;
	lash_config_t *config;

	gtk_client = (gtk_client_t *) data;

	while ((event = lash_get_event(gtk_client->lash_client))) {
		deal_with_event(gtk_client, event);
	}

	while ((config = lash_get_config(gtk_client->lash_client))) {
		add_config(gtk_client, config);
	}

	usleep(10000);

	return TRUE;
}

void
save_project_cb(GtkButton * button, void *data)
{
	lash_client_t *client;
	lash_event_t *event;

	client = (lash_client_t *) data;

	event = lash_event_new_with_type(LASH_Save);
	lash_send_event(client, event);
}

const char *
get_filename(gtk_client_t * gtk_client)
{
	static GtkWidget *dialog = NULL;
	static char *file = NULL;
	int response;

	if (!dialog)
		dialog = gtk_file_selection_new("Select file");

	if (file) {
		free(file);
		file = NULL;
	}

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);

	if (response != GTK_RESPONSE_OK)
		return NULL;

	file =
		lash_strdup(gtk_file_selection_get_filename
					(GTK_FILE_SELECTION(dialog)));
	return file;
}

void
save_file_cb(GtkButton * button, void *data)
{
	gtk_client_t *client;
	const char *fqn;

	client = (gtk_client_t *) data;

	fqn = get_filename(client);

	if (!fqn)
		return;

	save_file(client, fqn);
}

void
open_file_cb(GtkButton * button, void *data)
{
	gtk_client_t *client;
	const char *fqn;

	client = (gtk_client_t *) data;

	fqn = get_filename(client);

	if (!fqn)
		return;

	open_file(client, fqn);
}

void
quit_cb(GtkButton * button, void *data)
{
	gtk_main_quit();
}

gboolean
key_exists(gtk_client_t * gtk_client, const char *key)
{
	GtkTreeIter iter;
	GtkTreeModel *tree_model;
	gboolean found = FALSE;
	gchar *existing_key;

	tree_model = GTK_TREE_MODEL(gtk_client->configs);

	if (gtk_tree_model_get_iter_first(tree_model, &iter))
		do {
			gtk_tree_model_get(tree_model, &iter, KEY_COLUMN, &existing_key,
							   -1);

			if (strcmp(existing_key, key) == 0)
				found = TRUE;

			free(existing_key);

			if (found)
				return TRUE;
		}
		while (gtk_tree_model_iter_next(tree_model, &iter));

	return FALSE;
}

void
edit_data(gtk_client_t * client, GtkTreeIter * iter)
{
	char *key;
	char *value;
	gint response;

	gtk_tree_model_get(GTK_TREE_MODEL(client->configs), iter,
					   KEY_COLUMN, &key, VALUE_COLUMN, &value, -1);

	gtk_entry_set_text(GTK_ENTRY(client->config_editor_key), key);
	gtk_entry_set_text(GTK_ENTRY(client->config_editor_value), value);
	free(key);
	free(value);

	response = gtk_dialog_run(GTK_DIALOG(client->config_editor));
	gtk_widget_hide(client->config_editor);

	if (response == GTK_RESPONSE_ACCEPT) {
		gtk_list_store_set(client->configs, iter,
						   VALUE_COLUMN,
						   gtk_entry_get_text(GTK_ENTRY
											  (client->config_editor_value)),
						   -1);

		key =
			(char *)gtk_entry_get_text(GTK_ENTRY(client->config_editor_key));
		if (!key_exists(client, key))
			gtk_list_store_set(client->configs, iter, KEY_COLUMN, key, -1);
	}
}

gboolean
get_selected_iter(gtk_client_t * client, GtkTreeIter * iter)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(client->config_list));

	return gtk_tree_selection_get_selected(selection, &model, iter);
}

const char *
create_new_key(gtk_client_t * gtk_client)
{
	static char key[32];
	unsigned long i;
	gboolean found = TRUE;

	for (i = 1; found && sprintf(key, "data-%ld", i); i++) {
		found = key_exists(gtk_client, key);
	}

	return key;
}

void
add_cb(GtkButton * button, void *data)
{
	gtk_client_t *client;
	GtkTreeIter iter;
	const char *key;

	client = (gtk_client_t *) data;

	key = create_new_key(client);

	gtk_list_store_append(client->configs, &iter);

	gtk_list_store_set(client->configs, &iter,
					   KEY_COLUMN, key,
					   VALUE_COLUMN, "This is a data value", -1);
}

void
edit_cb(GtkButton * button, void *data)
{
	GtkTreeIter iter;

	gtk_client_t *client;

	client = (gtk_client_t *) data;

	if (!get_selected_iter(client, &iter))
		return;

	edit_data(client, &iter);
}

void
remove_cb(GtkButton * button, void *data)
{
	GtkTreeIter iter;
	gtk_client_t *client;

	client = (gtk_client_t *) data;

	if (!get_selected_iter(client, &iter))
		return;

	gtk_list_store_remove(client->configs, &iter);
}

gtk_client_t *
gtk_client_create(lash_client_t * lash_client)
{
	gtk_client_t *gtk_client;
	GtkWidget *main_box;
	GtkWidget *button_bar;
	GtkWidget *filename_frame;
	GtkWidget *filename_label;

	GtkWidget *config_and_file_pane;

	GtkWidget *config_frame;
	GtkWidget *config_box;
	GtkWidget *config_list_scroll;
	GtkCellRenderer *config_renderer;
	GtkTreeViewColumn *config_column;
	GtkWidget *config_button_box;
	GtkWidget *add_button;
	GtkWidget *edit_button;
	GtkWidget *remove_button;

	GtkWidget *file_frame;
	GtkWidget *file_scroll;
	GtkWidget *file_text;

	guint status_context;

	GtkWidget *config_editor_table;
	GtkWidget *config_editor_key_label;
	GtkWidget *config_editor_value_label;

	gtk_client = lash_malloc(sizeof(gtk_client_t));
	gtk_client->lash_client = lash_client;

	gtk_client->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gtk_client->window), WINDOW_TITLE);
	gtk_window_set_default_size(GTK_WINDOW(gtk_client->window), 450, 282);
	g_signal_connect(G_OBJECT(gtk_client->window), "delete_event",
					 G_CALLBACK(quit_cb), NULL);

	main_box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_box);
	gtk_container_add(GTK_CONTAINER(gtk_client->window), main_box);

	button_bar = gtk_toolbar_new();
	gtk_widget_show(button_bar);
	gtk_box_pack_start(GTK_BOX(main_box), button_bar, FALSE, TRUE, 0);

/*  gtk_toolbar_append_item (GTK_TOOLBAR (button_bar),
                           "Save Project",
                           "Tell the lash server to save the project",
                           "Tell the lash server to save the project",
                           NULL,
                           G_CALLBACK (save_project_cb),
                           lash_client);*/

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_SAVE,
							 "Tell the lash server to save the project",
							 "Tell the lash server to save the project",
							 G_CALLBACK(save_project_cb), lash_client, -1);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_SAVE_AS,
							 "Save the file data to a stand alone file",
							 "Save the file data to a stand alone file",
							 G_CALLBACK(save_file_cb), gtk_client, -1);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_OPEN,
							 "Open some file data",
							 "Open some file data",
							 G_CALLBACK(open_file_cb), gtk_client, -1);

	gtk_toolbar_append_space(GTK_TOOLBAR(button_bar));

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_QUIT,
							 "Quit the application",
							 "Quit the application",
							 G_CALLBACK(quit_cb), NULL, -1);

	/*
	 * filename frame
	 */
	filename_frame = gtk_frame_new("File name");
	gtk_widget_show(filename_frame);
	gtk_container_set_border_width(GTK_CONTAINER(filename_frame), 4);
	gtk_box_pack_start(GTK_BOX(main_box), filename_frame, FALSE, TRUE, 0);

	filename_label = gtk_label_new(filename);
	gtk_widget_show(filename_label);
	gtk_misc_set_padding(GTK_MISC(filename_label), 8, 4);
	gtk_misc_set_alignment(GTK_MISC(filename_label), 0.0, 0.5);
	gtk_container_add(GTK_CONTAINER(filename_frame), filename_label);

	/*
	 * config and file pane
	 */
	config_and_file_pane = gtk_vpaned_new();
	gtk_widget_show(config_and_file_pane);
	gtk_box_pack_start(GTK_BOX(main_box), config_and_file_pane, TRUE, TRUE,
					   0);

	/*
	 * config frame
	 */
	config_frame = gtk_frame_new("Configs");
	gtk_widget_show(config_frame);
	gtk_container_set_border_width(GTK_CONTAINER(config_frame), 4);
	gtk_paned_pack1(GTK_PANED(config_and_file_pane), config_frame, TRUE,
					FALSE);

	config_box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(config_box);
	gtk_container_add(GTK_CONTAINER(config_frame), config_box);

	config_list_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(config_list_scroll);
	gtk_box_pack_start(GTK_BOX(config_box), config_list_scroll, TRUE, TRUE,
					   0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(config_list_scroll),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	/* the list stuff */
	gtk_client->configs =
		gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	gtk_client->config_list =
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(gtk_client->configs));
	gtk_widget_show(gtk_client->config_list);
	gtk_container_add(GTK_CONTAINER(config_list_scroll),
					  gtk_client->config_list);

	/* key column */
	config_renderer = gtk_cell_renderer_text_new();
	config_column =
		gtk_tree_view_column_new_with_attributes("Key", config_renderer,
												 "text", KEY_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_client->config_list),
								config_column);

	/* value column */
	config_renderer = gtk_cell_renderer_text_new();
	config_column =
		gtk_tree_view_column_new_with_attributes("Value", config_renderer,
												 "text", VALUE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_client->config_list),
								config_column);

	/* the rest */
	config_button_box = gtk_vbutton_box_new();
	gtk_widget_show(config_button_box);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(config_button_box),
							  GTK_BUTTONBOX_START);
	gtk_box_pack_start(GTK_BOX(config_box), config_button_box, FALSE, TRUE,
					   0);

	add_button = gtk_button_new_with_label("Add");
	gtk_widget_show(add_button);
	g_signal_connect(G_OBJECT(add_button), "clicked",
					 G_CALLBACK(add_cb), gtk_client);
	gtk_box_pack_start(GTK_BOX(config_button_box), add_button, FALSE, TRUE,
					   0);

	edit_button = gtk_button_new_with_label("Edit");
	gtk_widget_show(edit_button);
	g_signal_connect(G_OBJECT(edit_button), "clicked",
					 G_CALLBACK(edit_cb), gtk_client);
	gtk_box_pack_start(GTK_BOX(config_button_box), edit_button, FALSE, TRUE,
					   0);

	remove_button = gtk_button_new_with_label("Remove");
	gtk_widget_show(remove_button);
	g_signal_connect(G_OBJECT(remove_button), "clicked",
					 G_CALLBACK(remove_cb), gtk_client);
	gtk_box_pack_start(GTK_BOX(config_button_box), remove_button, FALSE, TRUE,
					   0);

	/*
	 * file frame
	 */
	file_frame = gtk_frame_new("File data");
	gtk_widget_show(file_frame);
	gtk_container_set_border_width(GTK_CONTAINER(file_frame), 4);
	gtk_paned_pack2(GTK_PANED(config_and_file_pane), file_frame, TRUE, FALSE);

	file_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(file_scroll);
	gtk_container_add(GTK_CONTAINER(file_frame), file_scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(file_scroll),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	file_text = gtk_text_view_new();
	gtk_widget_show(file_text);
	gtk_container_add(GTK_CONTAINER(file_scroll), file_text);
	gtk_client->file_data =
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_text));

	/*
	 * status bar
	 */
	gtk_client->status_bar = gtk_statusbar_new();
	gtk_widget_show(gtk_client->status_bar);
	status_context =
		gtk_statusbar_get_context_id(GTK_STATUSBAR(gtk_client->status_bar),
									 "connected");
	gtk_statusbar_push(GTK_STATUSBAR(gtk_client->status_bar), status_context,
					   g_strdup_printf("Connected to server %s",
									   lash_get_server_name(lash_client)));
	gtk_box_pack_start(GTK_BOX(main_box), gtk_client->status_bar, FALSE, TRUE,
					   0);

	/*
	 * config editor
	 */
	gtk_client->config_editor =
		gtk_dialog_new_with_buttons("LASH Config Editor",
									GTK_WINDOW(gtk_client->window),
									GTK_DIALOG_MODAL |
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
									NULL);

	config_editor_table = gtk_table_new(2, 2, FALSE);
	gtk_widget_show(config_editor_table);
	gtk_container_add(GTK_CONTAINER
					  (GTK_DIALOG(gtk_client->config_editor)->vbox),
					  config_editor_table);
	gtk_container_set_border_width(GTK_CONTAINER(config_editor_table), 4);

	config_editor_key_label = gtk_label_new("Key");
	gtk_misc_set_alignment(GTK_MISC(config_editor_key_label), 0.5, 0.5);
	gtk_widget_show(config_editor_key_label);

	config_editor_value_label = gtk_label_new("Value");
	gtk_misc_set_alignment(GTK_MISC(config_editor_value_label), 0.5, 0.5);
	gtk_widget_show(config_editor_value_label);

	gtk_client->config_editor_key = gtk_entry_new();
	gtk_widget_show(gtk_client->config_editor_key);

	gtk_client->config_editor_value = gtk_entry_new();
	gtk_widget_show(gtk_client->config_editor_value);

	gtk_table_attach(GTK_TABLE(config_editor_table),
					 config_editor_key_label,
					 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_table_attach(GTK_TABLE(config_editor_table),
					 config_editor_value_label,
					 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
	gtk_table_attach(GTK_TABLE(config_editor_table),
					 gtk_client->config_editor_key,
					 1, 2, 0, 1,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 4, 0);
	gtk_table_attach(GTK_TABLE(config_editor_table),
					 gtk_client->config_editor_value,
					 1, 2, 1, 2,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 4, 0);

	gtk_idle_add(idle_cb, gtk_client);
	gtk_widget_show(gtk_client->window);

	return gtk_client;
}
