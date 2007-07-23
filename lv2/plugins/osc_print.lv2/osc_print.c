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
#include "lv2.h"
#include "../../extensions/osc/lv2_osc.h"

/* Plugin */


typedef struct {
	LV2OSCBuffer* input_buffer;
} OSCPrint;


static void osc_print_cleanup(LV2_Handle instance)
{
	free(instance);
}


static void osc_print_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	OSCPrint* plugin = (OSCPrint*)instance;

	switch (port) {
	case 0:
		plugin->input_buffer = data;
		break;
	}
}


static LV2_Handle
osc_print_instantiate(const LV2_Descriptor*         descriptor,
                      double                        rate,
                      const char*                   bundle_path,
                      const LV2_Host_Feature*const* features)
{
	OSCPrint* plugin = (OSCPrint*)malloc(sizeof(OSCPrint));

	return (LV2_Handle)plugin;
}


static void
osc_print_run(LV2_Handle instance, uint32_t sample_count)
{
	OSCPrint* plugin = (OSCPrint*)instance;

	if (plugin->input_buffer && plugin->input_buffer->message_count > 0)
		printf("OSC Message!\n");
}



/* Library */


static LV2_Descriptor *osc_print_descriptor = NULL;


static void
init_descriptor()
{
	osc_print_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	osc_print_descriptor->URI = "http://drobilla.net/lv2_plugins/osc_print/0";
	osc_print_descriptor->activate = NULL;
	osc_print_descriptor->cleanup = osc_print_cleanup;
	osc_print_descriptor->connect_port = osc_print_connect_port;
	osc_print_descriptor->deactivate = NULL;
	osc_print_descriptor->instantiate = osc_print_instantiate;
	osc_print_descriptor->run = osc_print_run;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!osc_print_descriptor)
		init_descriptor();

	switch (index) {
	case 0:
		return osc_print_descriptor;
	default:
		return NULL;
	}
}

