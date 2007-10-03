/* An OSC metronome LV2 plugin
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
#include "../lv2.h"
#include "../../extensions/osc/lv2_osc.h"

/* Plugin */


typedef struct {
	double        sample_rate;
	uint32_t      frames_since_last_tick;

	LV2OSCBuffer* input_sync;
	float*        input_bpm;
	LV2OSCBuffer* output_bang;
} OSCMetro;


static void osc_metro_cleanup(LV2_Handle instance)
{
	free(instance);
}


static void osc_metro_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	OSCMetro* plugin = (OSCMetro*)instance;

	switch (port) {
	case 0:
		plugin->input_sync = data;
		break;
	case 1:
		plugin->input_bpm = data;
		break;
	case 2:
		plugin->output_bang = data;
		break;
	}
}


static LV2_Handle
osc_metro_instantiate(const LV2_Descriptor*    descriptor,
                      double                   rate,
                      const char*              bundle_path,
                      const LV2_Feature*const* features)
{
	OSCMetro* plugin = (OSCMetro*)malloc(sizeof(OSCMetro));
	
	plugin->sample_rate            = rate;
	plugin->frames_since_last_tick = 0;
	plugin->input_sync             = NULL;
	plugin->input_bpm              = NULL;
	plugin->output_bang            = NULL;

	return (LV2_Handle)plugin;
}


static void
osc_metro_run(LV2_Handle instance, uint32_t sample_count)
{
	OSCMetro* plugin = (OSCMetro*)instance;

	const double   frames_per_minute = plugin->sample_rate * 60.0;
	const uint32_t frames_per_beat   = (uint32_t)(frames_per_minute / (*plugin->input_bpm));

	/* FIXME: write this correctly (sample accurate) */

	if (plugin->frames_since_last_tick > frames_per_beat) {
			
		//printf("Bang!\n");
	
		if (plugin->output_bang) {
			lv2_osc_buffer_append(plugin->output_bang, 0.0, "/*", NULL);
		}

		plugin->frames_since_last_tick = 0;

	} else {
		plugin->frames_since_last_tick += sample_count;
	}
}



/* Library */


static LV2_Descriptor *osc_metro_descriptor = NULL;


static void
init_descriptor()
{
	osc_metro_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	osc_metro_descriptor->URI = "http://drobilla.net/lv2_plugins/dev/osc_metronome";
	osc_metro_descriptor->activate = NULL;
	osc_metro_descriptor->cleanup = osc_metro_cleanup;
	osc_metro_descriptor->connect_port = osc_metro_connect_port;
	osc_metro_descriptor->deactivate = NULL;
	osc_metro_descriptor->instantiate = osc_metro_instantiate;
	osc_metro_descriptor->run = osc_metro_run;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!osc_metro_descriptor)
		init_descriptor();

	switch (index) {
	case 0:
		return osc_metro_descriptor;
	default:
		return NULL;
	}
}

