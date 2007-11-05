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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lv2.h"
#include "extensions/osc/lv2_osc.h"
#include "extensions/contexts/lv2_contexts.h"

static const char* message_context_uri = "http://drobilla.net/ns/lv2ext/contexts/MessageContext";

/* Plugin */


typedef struct {
	LV2OSCBuffer* input_buffer;
	LV2OSCBuffer* output_buffer;
} OSCPrint;


static LV2_Handle
osc_bang_instantiate(const LV2_Descriptor*    descriptor,
                     double                   rate,
                     const char*              bundle_path,
                     const LV2_Feature*const* features)
{
	OSCPrint* plugin = (OSCPrint*)malloc(sizeof(OSCPrint));
	
	plugin->input_buffer = NULL;
	plugin->output_buffer = NULL;

	return (LV2_Handle)plugin;
}


static void
osc_bang_cleanup(LV2_Handle instance)
{
	free(instance);
}
	

static LV2BlockingContext osc_bang_message_context_data;


static const void*
osc_bang_extension_data(const char* uri)
{
	if (!strcmp(uri, message_context_uri)) {
		return &osc_bang_message_context_data;
	} else {
		return NULL;
	}
}


static void
osc_bang_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	OSCPrint* plugin = (OSCPrint*)instance;

	switch (port) {
	case 0:
		plugin->input_buffer = data;
		break;
	case 1:
		plugin->output_buffer = data;
		break;
	}
}

static bool
osc_bang_blocking_run(LV2_Handle instance, uint8_t* outputs_written)
{
	printf("OSC_BANG BLOCKING RUN\n");
	/*
	OSCPrint* plugin = (OSCPrint*)instance;

	if (plugin->input_buffer && plugin->output_buffer) {

		for (uint32_t i=0; i < plugin->input_buffer->message_count; ++i) {

			const LV2Message* in = lv2_osc_buffer_get_message(plugin->input_buffer, i);
			lv2_osc_buffer_append(plugin->output_buffer, in->time, "/bang", NULL);

		}
		
		LV2_CONTEXTS_SET_OUTPUT_WRITTEN(outputs_written, 1);
		return true;
	
	} else {
		
		LV2_CONTEXTS_UNSET_OUTPUT_WRITTEN(outputs_written, 1);
		return false;

	}
	*/
	return false;
}

	
static void
osc_bang_run(LV2_Handle instance, uint32_t nframes)
{
	OSCPrint* plugin = (OSCPrint*)instance;

	if (plugin->input_buffer && plugin->output_buffer) {
		for (uint32_t i=0; i < plugin->input_buffer->message_count; ++i) {
			const LV2Message* in = lv2_osc_buffer_get_message(plugin->input_buffer, i);
			lv2_osc_buffer_append(plugin->output_buffer, in->time, "/bang", NULL);
		}
	}
}


/* Library */


static LV2_Descriptor *osc_bang_descriptor = NULL;


void
init_descriptor()
{
	osc_bang_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	osc_bang_descriptor->URI = "http://drobilla.net/lv2_plugins/dev/osc_bang";
	osc_bang_descriptor->activate = NULL;
	osc_bang_descriptor->cleanup = osc_bang_cleanup;
	osc_bang_descriptor->connect_port = osc_bang_connect_port;
	osc_bang_descriptor->deactivate = NULL;
	osc_bang_descriptor->instantiate = osc_bang_instantiate;
	osc_bang_descriptor->run = osc_bang_run;
	osc_bang_descriptor->extension_data = osc_bang_extension_data;

	osc_bang_message_context_data.blocking_run = osc_bang_blocking_run;
	osc_bang_message_context_data.connect_port = NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!osc_bang_descriptor)
		init_descriptor(); /* FIXME: leak */

	switch (index) {
	case 0:
		return osc_bang_descriptor;
	default:
		return NULL;
	}
}

