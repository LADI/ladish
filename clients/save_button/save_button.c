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

#include "config.h"

#include <gtk/gtk.h>
#include <lash/lash.h>

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GTK2
#  include "hints.h"
#endif

GtkWidget *window;
GtkWidget *button;
GtkWidget *menu;

int on_top = 1;
int sticky = 1;
int position = 1;

lash_client_t *lash_client;

#ifdef GTK2
const char *client_class = "LASH Save Button 2";
#else
const char *client_class = "LASH Save Button";
#endif

void
quit_cb()
{
	gtk_main_quit();
}

#ifndef GTK2
gboolean
motion_cb(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	int x, y;

	gdk_window_get_position(gtk_widget_get_parent_window(button), &x, &y);
	gdk_window_move(gtk_widget_get_parent_window(button), x + event->x,
					y + event->y);
	return TRUE;
}

gboolean
button_release_cb(GtkWidget * widget,
				  GdkEventButton * event, gpointer user_data)
{
	gtk_signal_disconnect_by_func(GTK_OBJECT(button),
								  GTK_SIGNAL_FUNC(motion_cb), NULL);
	gtk_grab_remove(button);
	return FALSE;
}
#endif /* ndef GTK2 */

gboolean
button_press_cb(GtkWidget * widget,
				GdkEventButton * event, gpointer user_data)
{
	switch (event->button) {
	case 2:
#ifdef GTK2
		gtk_window_begin_move_drag(GTK_WINDOW(window), 2,
								   event->x_root, event->y_root, event->time);
#else
		printf("middle button press\n");
		gtk_signal_connect(GTK_OBJECT(button), "motion-notify-event",
						   GTK_SIGNAL_FUNC(motion_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(button), "button-release-event",
						   GTK_SIGNAL_FUNC(button_release_cb), NULL);
		gtk_grab_add(button);
#endif
		return TRUE;
	case 3:
		quit_cb();
		return TRUE;
	}

	return FALSE;
}

void
clicked_cb(GtkButton * button, gpointer user_data)
{
	lash_event_t *event;

	event = lash_event_new_with_type(LASH_Save);
	lash_send_event(lash_client, event);
}

void
send_window_position()
{
	lash_config_t *config;
	int x, y;
	char pos[16];

#ifdef GTK2
	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
#else
	gdk_window_get_position(gtk_widget_get_parent_window(button), &x, &y);
#endif

	sprintf(pos, "%d %d", x, y);
	config = lash_config_new_with_key("window-position");
	lash_config_set_value_string(config, pos);

	lash_send_config(lash_client, config);
}

gboolean
idle_cb(void *data)
{
	lash_event_t *event;
	lash_config_t *config;

	while ((event = lash_get_event(lash_client))) {
		switch (lash_event_get_type(event)) {
		case LASH_Save_Data_Set:
			send_window_position();
			lash_send_event(lash_client, event);
			break;
		case LASH_Restore_Data_Set:
			lash_send_event(lash_client, event);
			break;
		case LASH_Server_Lost:
			gtk_main_quit();
			exit(0);
		default:
			fprintf(stderr, "%s: recieved unknown LASH event of type %d\n",
					__FUNCTION__, lash_event_get_type(event));
			lash_event_destroy(event);
			break;
		}
	}

	while ((config = lash_get_config(lash_client))) {
		if (position) {
			int x, y;

			sscanf(lash_config_get_value_string(config), "%d %d", &x, &y);
#ifdef GTK2
			gtk_window_move(GTK_WINDOW(window), x, y);
#else
			gdk_window_move(gtk_widget_get_parent_window(button), x, y);
#endif
		}
		lash_config_destroy(config);
	}

#ifdef GTK2
	if (on_top)
		gdk_window_raise(gtk_widget_get_parent_window(button));
#endif

	usleep(10000);

	return TRUE;
}

static void
print_help()
{
	printf("%s version %s\n", client_class, PACKAGE_VERSION);
	printf("Copyright (C) 2002 Robert Ham <rah@bash.sh>\n");
	printf("\n");
	printf
		("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
	printf
		("under the terms of the GNU General Public License, version 2 or later.  See\n");
	printf("the COPYING file that came with this software for details.\n");
	printf("\n");
	printf(" -h, --help                  Display this help info\n");
	printf
		(" -n, --not-on-top            Don't make the button stay atop windows\n");
	printf
		(" -u, --unsticky              Don't make the button sticky (window-manager wise)\n");
	printf
		(" -p, --no-position           Don't restore the window position\n");
	printf("     --lash-port=<port>    Connect to server on port <port>\n");
	printf
		("     --lash-project=<proj> Connect to the project named <proj>\n");
	printf("\n");
}

int
main(int argc, char **argv)
{
	lash_args_t *lash_args;
	int opt;
	const char *options = "hf:nup";
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"not-on-top", 0, NULL, 'n'},
		{"unsticky", 0, NULL, 'u'},
		{"no-position", 0, NULL, 'p'},
		{0, 0, 0, 0}
	};

	lash_args = lash_extract_args(&argc, &argv);

	gtk_set_locale();
	gtk_init(&argc, &argv);

	while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {
		switch (opt) {
		case 'n':
			on_top = 0;
			break;
		case 'u':
			sticky = 0;
			break;
		case 'p':
			position = 0;
			break;
		case 'h':
			print_help();
			exit(0);
			break;
		case ':':
		case '?':
			print_help();
			exit(1);
			break;
		}
	}

	lash_client =
		lash_init(lash_args, client_class, LASH_Config_Data_Set,
				  LASH_PROTOCOL(2, 0));
	if (!lash_client) {
		fprintf(stderr, "%s: could not initialise lash\n", __FUNCTION__);
		exit(1);
	}

/*  check_wm_hints (); */

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), client_class);
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
#ifdef GTK2
	g_signal_connect(G_OBJECT(window), "delete_event",
					 G_CALLBACK(quit_cb), NULL);
	if (on_top)
		gtk_window_set_type_hint(GTK_WINDOW(window),
								 GDK_WINDOW_TYPE_HINT_MENU);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
#else
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
					   GTK_SIGNAL_FUNC(quit_cb), NULL);

	gtk_widget_realize(window);
	gdk_window_set_decorations(window->window, 0);
#endif /* GTK2 */

	button = gtk_button_new_with_label("Save");
/*  GTK_WIDGET_UNSET_FLAGS ((button), (GTK_CAN_FOCUS)); */
	gtk_widget_show(button);
	gtk_container_add(GTK_CONTAINER(window), button);
#ifdef GTK2
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button), "button-press-event",
					 G_CALLBACK(button_press_cb), NULL);
#else
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
					   GTK_SIGNAL_FUNC(clicked_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(button), "button-press-event",
					   GTK_SIGNAL_FUNC(button_press_cb), NULL);
	gtk_widget_add_events(button, GDK_POINTER_MOTION_MASK);
#endif /* GTK2 */
	gtk_widget_show(window);

#ifdef GTK2
	if (sticky)
		gtk_window_stick(GTK_WINDOW(window));
#else
	check_wm_hints();

	if (sticky && hint_sticky_available())
		set_sticky_func(window, TRUE);

	if (on_top && hint_always_on_top_available()) {
		set_always_func(window, TRUE);
		if (hint_skip_winlist_available())
			set_skip_winlist_func(window);
	}
#endif

	gtk_idle_add(idle_cb, NULL);

	gtk_main();

	send_window_position();

	return 0;
}
