/* LVZ - A C++ interface for writing LV2 plugins.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 *  
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef UI_CLASS
#error "This file requires UI_CLASS to be defined"
#endif
#ifndef URI_PREFIX
#error "This file requires URI_PREFIX to be defined"
#endif
#ifndef UI_URI_SUFFIX
#error "This file requires UI_URI_SUFFIX to be defined"
#endif

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cassert>
#include "AEffEditor.hpp"
#include "lv2_ui.h"
#include UI_HEADER
#include PLUGIN_HEADER

extern "C" {

/* UI */

typedef struct {
	bool          stolen;
	UI_CLASS*     ui;
	GtkSocket*    socket;
	Window        x_window;
	AudioEffectX* effect;
} MDAPluginUI;


static gboolean
mda_ui_idle(void* data)
{
	MDAPluginUI* ui = (MDAPluginUI*)data;
	if (!ui->stolen) {
		//gtk_socket_add_id(GTK_SOCKET(ui->socket), ui->x_window);
		ui->x_window = (Window)gtk_socket_get_id(GTK_SOCKET(ui->socket));
		bool success = ui->ui->open((void*)ui->x_window);
		if (!success)
			fprintf(stderr, "FAILED TO OPEN GUI\n");
		ui->ui->getFrame()->draw();
		ui->stolen = true;
	}

	ui->ui->idle();
	return true;
}


static LV2UI_Handle
mda_ui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                   const char*                     plugin_uri,
                   const char*                     bundle_path,
                   LV2UI_Write_Function            write_function,
                   LV2UI_Controller                controller,
                   LV2UI_Widget*                   widget,
                   const LV2_Feature* const*       features)
{
	VSTGUI::setBundlePath(bundle_path);

	MDAPluginUI* ui = (MDAPluginUI*)malloc(sizeof(MDAPluginUI));
	ui->effect = NULL;

	typedef struct { const void* (*extension_data)(const char* uri); } LV2_DataAccess;
				
	typedef const void*         (*extension_data_func)(const char* uri);
	typedef const AudioEffectX* (*get_effect_func)(LV2_Handle instance);
	
	LV2_Handle      instance = NULL;
	get_effect_func get_effect = NULL;


	if (features != NULL) {
		const LV2_Feature* feature = features[0];
		for (size_t i = 0; (feature = features[i]) != NULL; ++i) {
			if (!strcmp(feature->URI, "http://lv2plug.in/ns/ext/instance-access")) {
				instance = (LV2_Handle)feature->data;
			} else if (!strcmp(feature->URI, "http://lv2plug.in/ns/ext/data-access")) {
				LV2_DataAccess* ext_data = (LV2_DataAccess*)feature->data;
				extension_data_func func = (extension_data_func)feature->data;
				get_effect = (get_effect_func)ext_data->extension_data(
						"http://drobilla.net/ns/dev/vstgui");
			}
		}
	}

	if (instance && get_effect) {
		ui->effect = (AudioEffectX*)get_effect(instance);
	} else {
		fprintf(stderr, "Host does not support required features, aborting.\n");
		return NULL;
	}

	ui->ui = new UI_CLASS(ui->effect);
	ui->stolen = false;

	ui->socket = GTK_SOCKET(gtk_socket_new());
	gtk_widget_show_all(GTK_WIDGET(ui->socket));
	
	*widget = ui->socket;
	g_timeout_add(30, mda_ui_idle, ui);
	
	return ui;
}


static void
mda_ui_cleanup(LV2UI_Handle instance)
{
	g_idle_remove_by_data(instance);
}


static void
mda_ui_port_event(LV2UI_Handle ui,
                  uint32_t     port_index,
                  uint32_t     buffer_size,
                  uint32_t     format,
                  const void*  buffer)
{
	// VST UIs seem to not use this at all, it's all polling
	// The shit the proprietary people come up (and get away) with...
}
 

static const void*
mda_ui_extension_data(const char* uri)
{
	return NULL;
}


/* Library */

static LV2UI_Descriptor *mda_ui_descriptor = NULL;

static void
init_ui_descriptor()
{
	mda_ui_descriptor = (LV2UI_Descriptor*)malloc(sizeof(LV2UI_Descriptor));

	mda_ui_descriptor->URI = URI_PREFIX UI_URI_SUFFIX;
	mda_ui_descriptor->instantiate = mda_ui_instantiate;
	mda_ui_descriptor->cleanup = mda_ui_cleanup;
	mda_ui_descriptor->port_event = mda_ui_port_event;
	mda_ui_descriptor->extension_data = mda_ui_extension_data;
}


LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	if (!mda_ui_descriptor)
		init_ui_descriptor();

	switch (index) {
	case 0:
		return mda_ui_descriptor;
	default:
		return NULL;
	}
}


LV2_SYMBOL_EXPORT
AEffEditor*
lvz_new_aeffeditor(AudioEffect* effect)
{
	UI_CLASS* ui = new UI_CLASS(effect);
	ui->setURI(URI_PREFIX UI_URI_SUFFIX);
	ui->setPluginURI(URI_PREFIX PLUGIN_URI_SUFFIX);
	return ui;
}


} // extern "C"

