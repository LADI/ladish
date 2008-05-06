/* An LV2 plugin which outputs an OSC bang when it receives any message.
 * Copyright (C) 2007 Dave Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "lv2/ui/lv2_ui.h"
#include "lv2/osc/lv2_osc.h"

/* UI */


typedef struct {
	LV2UI_Controller     controller;
	LV2UI_Write_Function write_function;
	GtkWidget*           button;
} OSCBangUI;
 

void
bang_ui_on_click(GtkWidget* widget, void* data)
{
	//OSCBangUI* ui = (OSCBangUI*)data;
	printf("CLICK\n");
	//LV2Message* msg = lv2_message_new(0, "/bang", NULL);
	//ui->write_function(ui->controller, 0, lv2_message_get_size(msg), msg);
	//free(msg);
}


LV2UI_Handle
bang_ui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                        const char*                     plugin_uri,
                        const char*                     bundle_path,
                        LV2UI_Write_Function            write_function,
                        LV2UI_Controller                controller,
                        LV2UI_Widget*                   widget,
                        const LV2_Feature* const*       features)
{
	OSCBangUI* ui = (OSCBangUI*)malloc(sizeof(OSCBangUI));
	
	ui->button = gtk_button_new_with_label("Bang");
	g_object_ref(ui->button);

	gtk_signal_connect(GTK_OBJECT(ui->button), "clicked",
			GTK_SIGNAL_FUNC(bang_ui_on_click), ui);

	*widget = ui->button;

	ui->controller     = controller;
	ui->write_function = write_function;

	return (LV2UI_Handle)ui;
}


void
bang_ui_cleanup(LV2UI_Handle ui)
{
	g_object_unref(((OSCBangUI*)ui)->button);
}


/* Library */


static LV2UI_Descriptor *bang_ui_descriptor = NULL;


void
init_descriptor()
{
	bang_ui_descriptor = (LV2UI_Descriptor*)malloc(sizeof(LV2UI_Descriptor));

	bang_ui_descriptor->URI = "http://drobilla.net/lv2_plugins/dev/bang_ui";
	bang_ui_descriptor->instantiate = bang_ui_instantiate;
	bang_ui_descriptor->cleanup = bang_ui_cleanup;
	bang_ui_descriptor->port_event = NULL;
	bang_ui_descriptor->extension_data = NULL;
}


LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	if (!bang_ui_descriptor)
		init_descriptor(); /* FIXME: leak */

	switch (index) {
	case 0:
		return bang_ui_descriptor;
	default:
		return NULL;
	}
}

