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
#include "lv2/event/lv2_event.h"
#include "lv2/event/lv2_event_helpers.h"
#include "lv2/contexts/lv2_contexts.h"


/* Plugin */


typedef struct {
	LV2_Event_Buffer* input_buffer;
	LV2_Event_Buffer* output_buffer;
} Bang;


static LV2_Handle
bang_instantiate(const LV2_Descriptor*    descriptor,
                     double                   rate,
                     const char*              bundle_path,
                     const LV2_Feature*const* features)
{
	Bang* plugin = (Bang*)malloc(sizeof(Bang));
	
	plugin->input_buffer = NULL;
	plugin->output_buffer = NULL;

	return (LV2_Handle)plugin;
}


static void
bang_cleanup(LV2_Handle instance)
{
	free(instance);
}
	

//static LV2BlockingContext bang_message_context_data;


#if 0
static const void*
bang_extension_data(const char* uri)
{
	if (!strcmp(uri, LV2_CONTEXT_MESSAGE)) {
		return &bang_message_context_data;
	} else {
		return NULL;
	}
}
#endif


static void
bang_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	Bang* plugin = (Bang*)instance;

	switch (port) {
	case 0:
		plugin->input_buffer = data;
		break;
	case 1:
		plugin->output_buffer = data;
		break;
	}
}

#if 0
static bool
bang_msg_run(LV2_Handle instance, uint8_t* outputs_written)
{
	printf("BANG MESSAGE RUN\n");
	/*
	Bang* plugin = (Bang*)instance;

	if (plugin->input_buffer && plugin->output_buffer) {

		for (uint32_t i=0; i < plugin->input_buffer->message_count; ++i) {

			const LV2Message* in = lv2_buffer_get_message(plugin->input_buffer, i);
			lv2_buffer_append(plugin->output_buffer, in->time, "/bang", NULL);

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
#endif

	
static void
bang_run(LV2_Handle instance, uint32_t nframes)
{
	Bang* plugin = (Bang*)instance;

	if (plugin->input_buffer && plugin->output_buffer) {
		LV2_Event_Iterator in;
		LV2_Event_Iterator out;
		uint8_t*           data;

		lv2_event_begin(&in, plugin->input_buffer);
		lv2_event_begin(&out, plugin->output_buffer);

		// Copy input events directly to output
		// FIXME: Not really what this plugin should do....
		while (lv2_event_is_valid(&in)) {
			const LV2_Event* ev = lv2_event_get(&in, &data);
			lv2_event_write_event(&out, ev, data);
			lv2_event_increment(&in);
		}
	}
}

/* Library */


static LV2_Descriptor *bang_descriptor = NULL;


void
init_descriptor()
{
	bang_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	bang_descriptor->URI = "http://drobilla.net/lv2_plugins/dev/bang";
	bang_descriptor->activate = NULL;
	bang_descriptor->cleanup = bang_cleanup;
	bang_descriptor->connect_port = bang_connect_port;
	bang_descriptor->deactivate = NULL;
	bang_descriptor->instantiate = bang_instantiate;
	bang_descriptor->run = bang_run;
	bang_descriptor->extension_data = NULL; /*bang_extension_data;

	bang_message_context_data.blocking_run = bang_blocking_run;
	bang_message_context_data.connect_port = bang_blocking_connect_port;*/
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!bang_descriptor)
		init_descriptor(); /* FIXME: leak */

	switch (index) {
	case 0:
		return bang_descriptor;
	default:
		return NULL;
	}
}

