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

#include "config.h"

#ifdef HAVE_GTK2

#  define _GNU_SOURCE

#  include <unistd.h>

#  include <gtk/gtk.h>

#  include "interface.h"
#  include "synth.h"
#  include "lash.h"

#  define GAIN_UPPER       10.0
#  define MODULATION_UPPER 50.0
#  define ENVELOPE_UPPER   1.0

static gboolean show_toolbar = TRUE;
GtkWidget *adsr_display = NULL;

GtkWidget *modulation_range;
GtkWidget *attack_range;
GtkWidget *decay_range;
GtkWidget *sustain_range;
GtkWidget *release_range;
GtkWidget *gain_range;
GtkWidget *harmonic_spin;
GtkWidget *subharmonic_spin;
GtkWidget *transpose_spin;

int interface_up = 0;

gboolean
quit_cb()
{
	quit = 1;
	gtk_main_quit();
	return TRUE;
}

gboolean
idle_cb(void *data)
{
	if (quit) {
		gtk_main_quit();
		return FALSE;
	}

	if (lash_enabled(lash_client))
		lash_main();

	gtk_range_set_value(GTK_RANGE(modulation_range), modulation);
	gtk_range_set_value(GTK_RANGE(attack_range), attack);
	gtk_range_set_value(GTK_RANGE(decay_range), decay);
	gtk_range_set_value(GTK_RANGE(sustain_range), sustain);
	gtk_range_set_value(GTK_RANGE(release_range), release);
	gtk_range_set_value(GTK_RANGE(gain_range), gain);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(harmonic_spin), harmonic);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(subharmonic_spin), subharmonic);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(transpose_spin), transpose);

	usleep(1000);

	return TRUE;
}

void
save_cb()
{
	lash_event_t *event;

	event = lash_event_new_with_type(LASH_Save);
	lash_send_event(lash_client, event);
}

gboolean
toggle_button_bar_cb(GtkWidget * widget, GdkEventButton * event,
					 gpointer data)
{
	GtkWidget *handle;

	handle = GTK_WIDGET(data);

	show_toolbar = !show_toolbar;
	if (show_toolbar)
		gtk_widget_show(handle);
	else
		gtk_widget_hide(handle);

	return TRUE;
}

void
range_cb(GtkRange * range, gpointer data)
{
	double *val;

	val = (double *)data;
	*val = gtk_range_get_value(range);

	gtk_widget_queue_draw_area(adsr_display, 0, 0,
							   adsr_display->allocation.width,
							   adsr_display->allocation.height);
}

void
spin_cb(GtkSpinButton * spinbutton, gpointer data)
{
	int *val;

	val = (int *)data;
	*val = gtk_spin_button_get_value(spinbutton);
}

gboolean
envelope_display_expose_cb(GtkWidget * widget, GdkEventExpose * event,
						   gpointer data)
{
	GdkDrawable *window;
	GdkGC *gc;
	gint envelope_width;
	gint envelope_height;
	gint attack_width;
	gint attack_height;
	gint decay_width;
	gint sustain_width;
	gint sustain_height;
	gint release_width;
	gdouble total;

	/* get all the drawing bits */
	window = widget->window;
	gc = widget->style->fg_gc[GTK_WIDGET_STATE(widget)];

	envelope_height = widget->allocation.height;
	envelope_width = widget->allocation.width;

	/* calculate the display variables */
	total = attack + decay + release;
	total += (total / 3);
	attack_width = (envelope_width / total) * attack;
	decay_width = (envelope_width / total) * decay;
	release_width = (envelope_width / total) * release;
	sustain_width =
		(envelope_width / total) * (total - attack - decay - release);

	attack_height = 0;
	sustain_height = envelope_height - envelope_height * sustain;

	/* do the drawing */
	gdk_draw_line(window, gc,
				  0, envelope_height, attack_width, attack_height);

	gdk_draw_line(window, gc,
				  attack_width, attack_height,
				  attack_width + decay_width, sustain_height);

	gdk_draw_line(window, gc,
				  attack_width + decay_width, sustain_height,
				  attack_width + decay_width + sustain_width, sustain_height);

	gdk_draw_line(window, gc,
				  attack_width + decay_width + sustain_width, sustain_height,
				  envelope_width, envelope_height);

	return TRUE;
}

void *
interface_main(void *data)
{
	/* for misc ops */
	GtkWidget *label;
	GtkWidget *box;

	GtkWidget *window;
	GtkWidget *main_box;
	GtkWidget *button_bar_handle;
	GtkWidget *button_bar;
	GtkWidget *synthesis_frame;
	GtkWidget *synthesis_box;
	GtkWidget *envelope_frame;
	GtkWidget *envelope_table;

	/* window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), alsa_client_name);
	gtk_window_set_default_size(GTK_WINDOW(window), 340, 370);
	g_signal_connect(G_OBJECT(window), "delete_event",
					 G_CALLBACK(quit_cb), NULL);

	/* main box */
	main_box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_box);
	gtk_container_add(GTK_CONTAINER(window), main_box);

	/* toolbar */

	button_bar_handle = gtk_handle_box_new();
	gtk_widget_show(button_bar_handle);
	gtk_box_pack_start(GTK_BOX(main_box), button_bar_handle, FALSE, TRUE, 0);

	button_bar = gtk_toolbar_new();
	gtk_widget_show(button_bar);
	gtk_container_add(GTK_CONTAINER(button_bar_handle), button_bar);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_SAVE,
							 "Tell the lash server to save the project",
							 "Tell the lash server to save the project",
							 G_CALLBACK(save_cb), NULL, -1);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(button_bar),
							 GTK_STOCK_QUIT,
							 "Close the synth",
							 "Close the synth",
							 G_CALLBACK(quit_cb), NULL, -1);

	/* 
	 * Synthesis frame
	 */
	synthesis_frame = gtk_frame_new("Synthesis");
	gtk_widget_show(synthesis_frame);
	gtk_container_set_border_width(GTK_CONTAINER(synthesis_frame), 4);
	gtk_box_pack_start(GTK_BOX(main_box), synthesis_frame, FALSE, TRUE, 0);

	/* but the label into an event box to get the button clicks */
	label = gtk_label_new("Synthesis");
	gtk_widget_show(label);

	box = gtk_event_box_new();
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER(box), label);
	gtk_widget_add_events(box, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(box), "button-press-event",
					 G_CALLBACK(toggle_button_bar_cb), button_bar_handle);
	gtk_frame_set_label_widget(GTK_FRAME(synthesis_frame), box);

	synthesis_box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(synthesis_box);
	gtk_container_add(GTK_CONTAINER(synthesis_frame), synthesis_box);

	box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(box);
	gtk_box_pack_start(GTK_BOX(synthesis_box), box, TRUE, TRUE, 0);

	/* modulation */
	box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(box);
	gtk_box_pack_start(GTK_BOX(synthesis_box), box, TRUE, TRUE, 4);

	label = gtk_label_new("Modulation");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	modulation_range =
		gtk_hscale_new_with_range(0.0, MODULATION_UPPER,
								  MODULATION_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(modulation_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(modulation_range), 5);
	gtk_range_set_value(GTK_RANGE(modulation_range), modulation);
	g_signal_connect(G_OBJECT(modulation_range), "value-changed",
					 G_CALLBACK(range_cb), &modulation);
	gtk_widget_show(modulation_range);
	gtk_box_pack_start(GTK_BOX(box), modulation_range, TRUE, TRUE, 0);

	box = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(box);
	gtk_box_pack_start(GTK_BOX(synthesis_box), box, TRUE, TRUE, 0);

	/* harmonic */
	label = gtk_label_new("Harmonic");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	harmonic_spin = gtk_spin_button_new_with_range(1.0, 100.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(harmonic_spin), 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(harmonic_spin), harmonic);
	g_signal_connect(G_OBJECT(harmonic_spin), "value-changed",
					 G_CALLBACK(spin_cb), &harmonic);
	gtk_widget_show(harmonic_spin);
	gtk_box_pack_start(GTK_BOX(box), harmonic_spin, TRUE, TRUE, 0);

	/* subharmonic */
	label = gtk_label_new("Subharmonic");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	subharmonic_spin = gtk_spin_button_new_with_range(1.0, 100.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(subharmonic_spin), 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(subharmonic_spin), subharmonic);
	g_signal_connect(G_OBJECT(subharmonic_spin), "value-changed",
					 G_CALLBACK(spin_cb), &subharmonic);
	gtk_widget_show(subharmonic_spin);
	gtk_box_pack_start(GTK_BOX(box), subharmonic_spin, TRUE, TRUE, 0);

	/* transpose */
	label = gtk_label_new("Transpose");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	transpose_spin = gtk_spin_button_new_with_range(-144.0, 144.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(transpose_spin), 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(transpose_spin), transpose);
	g_signal_connect(G_OBJECT(transpose_spin), "value-changed",
					 G_CALLBACK(spin_cb), &transpose);
	gtk_widget_show(transpose_spin);
	gtk_box_pack_start(GTK_BOX(box), transpose_spin, TRUE, TRUE, 0);

	/* gain */
	box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(box);
	gtk_box_pack_start(GTK_BOX(synthesis_box), box, TRUE, TRUE, 4);

	label = gtk_label_new("Gain");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	gain_range =
		gtk_hscale_new_with_range(0.0, GAIN_UPPER, GAIN_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(gain_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(gain_range), 5);
	gtk_range_set_value(GTK_RANGE(gain_range), gain);
	g_signal_connect(G_OBJECT(gain_range), "value-changed",
					 G_CALLBACK(range_cb), &gain);
	gtk_widget_show(gain_range);
	gtk_box_pack_start(GTK_BOX(box), gain_range, TRUE, TRUE, 0);

	/*
	 * envelope frame
	 */
	envelope_frame = gtk_frame_new("Envelope");
	gtk_widget_show(envelope_frame);
	gtk_container_set_border_width(GTK_CONTAINER(envelope_frame), 4);
	gtk_box_pack_start(GTK_BOX(main_box), envelope_frame, TRUE, TRUE, 0);

	box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER(envelope_frame), box);

	/* adsr display */
	adsr_display = gtk_drawing_area_new();
	gtk_widget_show(adsr_display);
/*  gtk_widget_set_size_request (adsr_display, 300, 130); */
	g_signal_connect(G_OBJECT(adsr_display), "expose_event",
					 G_CALLBACK(envelope_display_expose_cb), NULL);
	gtk_box_pack_start(GTK_BOX(box), adsr_display, TRUE, TRUE, 1);

	/* adsr controls */
	envelope_table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(envelope_table);
	gtk_box_pack_start(GTK_BOX(box), envelope_table, FALSE, TRUE, 0);

	/* attack */
	label = gtk_label_new("Attack");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(envelope_table), label,
					 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);

	attack_range =
		gtk_hscale_new_with_range(0.0, ENVELOPE_UPPER, ENVELOPE_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(attack_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(attack_range), 5);
	gtk_range_set_value(GTK_RANGE(attack_range), attack);
	g_signal_connect(G_OBJECT(attack_range), "value-changed",
					 G_CALLBACK(range_cb), &attack);
	gtk_widget_show(attack_range);
	gtk_table_attach(GTK_TABLE(envelope_table), attack_range,
					 1, 2, 0, 1,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 2);

	/* decay */
	label = gtk_label_new("Decay");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(envelope_table), label,
					 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 0);

	decay_range =
		gtk_hscale_new_with_range(0.0, ENVELOPE_UPPER, ENVELOPE_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(decay_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(decay_range), 5);
	gtk_range_set_value(GTK_RANGE(decay_range), decay);
	g_signal_connect(G_OBJECT(decay_range), "value-changed",
					 G_CALLBACK(range_cb), &decay);
	gtk_widget_show(decay_range);
	gtk_table_attach(GTK_TABLE(envelope_table), decay_range,
					 1, 2, 1, 2,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 2);

	/* sustain */
	label = gtk_label_new("Sustain");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(envelope_table), label,
					 0, 1, 2, 3, GTK_FILL, GTK_FILL, 4, 0);

	sustain_range =
		gtk_hscale_new_with_range(0.0, ENVELOPE_UPPER, ENVELOPE_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(sustain_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(sustain_range), 5);
	gtk_range_set_value(GTK_RANGE(sustain_range), sustain);
	g_signal_connect(G_OBJECT(sustain_range), "value-changed",
					 G_CALLBACK(range_cb), &sustain);
	gtk_widget_show(sustain_range);
	gtk_table_attach(GTK_TABLE(envelope_table), sustain_range,
					 1, 2, 2, 3,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 2);

	/* release */
	label = gtk_label_new("Release");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(envelope_table), label,
					 0, 1, 3, 4, GTK_FILL, GTK_FILL, 4, 0);

	release_range =
		gtk_hscale_new_with_range(0.0, ENVELOPE_UPPER, ENVELOPE_UPPER / 1000);
	gtk_scale_set_value_pos(GTK_SCALE(release_range), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(release_range), 5);
	gtk_range_set_value(GTK_RANGE(release_range), release);
	g_signal_connect(G_OBJECT(release_range), "value-changed",
					 G_CALLBACK(range_cb), &release);
	gtk_widget_show(release_range);
	gtk_table_attach(GTK_TABLE(envelope_table), release_range,
					 1, 2, 3, 4,
					 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 2);

	gtk_widget_show(window);

	gtk_idle_add(idle_cb, NULL);

	printf("Interface running\n");
	interface_up = 1;
	gtk_main();

	printf("Interface finished\n");
	return NULL;
}

#endif /* HAVE_GTK2 */
