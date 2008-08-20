/* An LV2 plugin which pretty-prints received OSC messages to stdout.
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
#include "lv2/osc/lv2_osc.h"
#include "lv2/osc/lv2_osc_print.h"
#include "lv2/contexts/lv2_contexts.h"

static const char* message_context_uri = "http://drobilla.net/ns/lv2ext/contexts/MessageContext";

/* Plugin */


typedef struct {
	LV2_Event_Buffer* input_buffer;
} Print;


static LV2_Handle
osc_print_instantiate(const LV2_Descriptor*    descriptor,
                      double                   rate,
                      const char*              bundle_path,
                      const LV2_Feature*const* features)
{
	Print* plugin = (Print*)malloc(sizeof(Print));
	
	plugin->input_buffer = NULL;

	return (LV2_Handle)plugin;
}


static void
osc_print_cleanup(LV2_Handle instance)
{
	free(instance);
}
	

static LV2BlockingContext osc_print_message_context_data;


static const void*
osc_print_extension_data(const char* uri)
{
	if (!strcmp(uri, message_context_uri)) {
		return &osc_print_message_context_data;
	} else {
		return NULL;
	}
}


static void
osc_print_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	Print* plugin = (Print*)instance;

	switch (port) {
	case 0:
		plugin->input_buffer = data;
		break;
	}
}


static bool
osc_print_blocking_run(LV2_Handle instance, uint8_t* outputs_written)
{
	Print* plugin = (Print*)instance;

	if (plugin->input_buffer) {

		//for (uint32_t i=0; i < plugin->input_buffer->event_count; ++i)
		//	lv2_osc_message_print(lv2_osc_buffer_get_message(plugin->input_buffer, i));
		
		LV2_CONTEXTS_SET_OUTPUT_WRITTEN(outputs_written, 0);
		return true;
	
	} else {
		LV2_CONTEXTS_UNSET_OUTPUT_WRITTEN(outputs_written, 0);
		return false;
	}
}


/* Library */


static LV2_Descriptor *osc_print_descriptor = NULL;

#if 0
/* Doesn't work :/ */
void init_descriptor() __attribute__((constructor));
#endif

void
init_descriptor()
{
	osc_print_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	osc_print_descriptor->URI = "http://drobilla.net/lv2_plugins/dev/osc_print";
	osc_print_descriptor->activate = NULL;
	osc_print_descriptor->cleanup = osc_print_cleanup;
	osc_print_descriptor->connect_port = osc_print_connect_port;
	osc_print_descriptor->deactivate = NULL;
	osc_print_descriptor->instantiate = osc_print_instantiate;
	osc_print_descriptor->run = NULL;
	osc_print_descriptor->extension_data = osc_print_extension_data;

	osc_print_message_context_data.blocking_run = osc_print_blocking_run;
	osc_print_message_context_data.connect_port = NULL;
}


#if 0
/* Doesn't work :/ */
static void
__attribute__((destructor))
free_descriptor()
{
	fprintf(stderr, "FINISH\n");
	free(osc_print_descriptor);
	osc_print_descriptor = NULL;
}
#endif


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!osc_print_descriptor)
		init_descriptor(); /* FIXME: leak */

	switch (index) {
	case 0:
		return osc_print_descriptor;
	default:
		return NULL;
	}
}

