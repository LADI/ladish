/* Sample and Hold.  Copyright (C) 2005 Thorsten Wilms.
 * Based on Dave Robillard's "Hz to AMS style V/Oct" plugin for the skeleton.
 * 
 * This plugin is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>

#include "ladspa.h"

#define SH_ID 4430

#define SH_NUM_PORTS 5

/* Port Numbers */
#define SH_INPUT      0
#define SH_TRIGGER    1
#define SH_THRESHOLD  2
#define SH_CONTINUOUS 3
#define SH_OUTPUT     4


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* input_buffer;
	LADSPA_Data* trigger_buffer;
	LADSPA_Data* threshold_buffer;
	LADSPA_Data* continuous_buffer;
	LADSPA_Data* output_buffer;

	float hold; /* the value sampled and held */
	float last_trigger; /* trigger port value from the previous frame */
} SH;


/* Construct a new plugin instance */
LADSPA_Handle
SH_instantiate(const LADSPA_Descriptor * descriptor, unsigned long srate)
{
	return (LADSPA_Handle)malloc(sizeof(SH));
}


/* Connect a port to a data location */
void
SH_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	SH* plugin = (SH*)instance;

	switch (port) {
	case SH_INPUT:
		plugin->input_buffer = location;
		break;
	case SH_TRIGGER:
		plugin->trigger_buffer = location;
		break;
	case SH_THRESHOLD:
		plugin->threshold_buffer = location;
		break;
	case SH_CONTINUOUS:
		plugin->continuous_buffer = location;
		break;
	case SH_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}

void
SH_activate(LADSPA_Handle instance)
{
	SH* plugin = (SH*)instance;

	plugin->hold = 0;
	plugin->last_trigger = 0;
}

void
SH_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	SH* const                plugin       = (SH*)instance;
	const LADSPA_Data* const input        = plugin->input_buffer;
	const LADSPA_Data* const trigger      = plugin->trigger_buffer;
	const LADSPA_Data        threshold    = *plugin->threshold_buffer;
	const LADSPA_Data        continuous   = *plugin->continuous_buffer;
	LADSPA_Data* const       output       = plugin->output_buffer;

	unsigned long i;

	/* Continuous triggering on (sample while trigger > threshold) */
	if (continuous != 0.0f) {
		for (i = 0; i < nframes; i++) {
			if (trigger[i] >= threshold)
				plugin->hold = input[i];
			plugin->last_trigger = trigger[i];
			output[i] = plugin->hold;
		}
	
	/* Continuous triggering off
	 * (only sample on first frame with trigger > threshold) */
	} else {
		for (i = 0; i < nframes; i++) {
			if (plugin->last_trigger < threshold && trigger[i] >= threshold)
				plugin->hold = input[i];
			plugin->last_trigger = trigger[i];
			output[i] = plugin->hold;
		}
	}
}


void
SH_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	SH* const                plugin       = (SH*)instance;
	const LADSPA_Data* const input        = plugin->input_buffer;
	const LADSPA_Data* const trigger      = plugin->trigger_buffer;
	const LADSPA_Data* const threshold    = plugin->threshold_buffer;
	const LADSPA_Data* const continuous   = plugin->continuous_buffer;
	LADSPA_Data* const       output       = plugin->output_buffer;

	unsigned long i;

	for (i = 0; i < nframes; i++) {
		if (continuous[0] != 0) {
			/* Continuous triggering on (sample while trigger > threshold) */
			if (trigger[i] >= threshold[i])
				plugin->hold = input[i];
		} else {
			/* Continuous triggering off
			 * (only sample on first frame with trigger > threshold) */
			if (plugin->last_trigger < threshold[i] && trigger[i] >= threshold[i])
				plugin->hold = input[i];
		}

		plugin->last_trigger = trigger[i];
		output[i] = plugin->hold;
	}
}


void
SH_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* SH_cr_desc = NULL;
LADSPA_Descriptor* SH_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	SH_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	SH_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (SH_cr_desc) {
		SH_cr_desc->UniqueID = SH_ID;
		SH_cr_desc->Label = strdup("sh_cr");
		SH_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		SH_cr_desc->Name = strdup("Sample and Hold (CR Threshold)");
		SH_cr_desc->Maker = strdup("Thorsten Wilms");
		SH_cr_desc->Copyright = strdup("GPL");
		SH_cr_desc->PortCount = SH_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(SH_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		SH_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[SH_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SH_TRIGGER] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SH_THRESHOLD] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[SH_CONTINUOUS] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[SH_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(SH_NUM_PORTS, sizeof(char*));
		SH_cr_desc->PortNames = (const char**)port_names;
		port_names[SH_INPUT] = strdup("Input");
		port_names[SH_TRIGGER] = strdup("Trigger");
		port_names[SH_THRESHOLD] = strdup("Threshold");
		port_names[SH_CONTINUOUS] = strdup("Continuous Triggering");
		port_names[SH_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(SH_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		SH_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[SH_INPUT].HintDescriptor = 0;
		port_range_hints[SH_TRIGGER].HintDescriptor = 0;
		port_range_hints[SH_THRESHOLD].HintDescriptor = 0;
		port_range_hints[SH_CONTINUOUS].HintDescriptor = LADSPA_HINT_TOGGLED;
		port_range_hints[SH_OUTPUT].HintDescriptor = 0;
		SH_cr_desc->instantiate = SH_instantiate;
		SH_cr_desc->connect_port = SH_connect_port;
		SH_cr_desc->activate = SH_activate;
		SH_cr_desc->run = SH_run_cr;
		SH_cr_desc->run_adding = NULL;
		SH_cr_desc->set_run_adding_gain = NULL;
		SH_cr_desc->deactivate = NULL;
		SH_cr_desc->cleanup = SH_cleanup;
	}

	if (SH_ar_desc) {
		SH_ar_desc->UniqueID = SH_ID+1;
		SH_ar_desc->Label = strdup("sh_ar");
		SH_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		SH_ar_desc->Name = strdup("Sample and Hold (AR Threshold)");
		SH_ar_desc->Maker = strdup("Thorsten Wilms");
		SH_ar_desc->Copyright = strdup("GPL");
		SH_ar_desc->PortCount = SH_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(SH_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		SH_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[SH_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SH_TRIGGER] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SH_THRESHOLD] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SH_CONTINUOUS] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[SH_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(SH_NUM_PORTS, sizeof(char*));
		SH_ar_desc->PortNames = (const char**)port_names;
		port_names[SH_INPUT] = strdup("Input");
		port_names[SH_TRIGGER] = strdup("Trigger");
		port_names[SH_THRESHOLD] = strdup("Threshold");
		port_names[SH_CONTINUOUS] = strdup("Continuous Triggering");
		port_names[SH_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(SH_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		SH_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[SH_INPUT].HintDescriptor = 0;
		port_range_hints[SH_TRIGGER].HintDescriptor = 0;
		port_range_hints[SH_THRESHOLD].HintDescriptor = 0;
		port_range_hints[SH_CONTINUOUS].HintDescriptor = LADSPA_HINT_TOGGLED;
		port_range_hints[SH_OUTPUT].HintDescriptor = 0;
		SH_ar_desc->instantiate = SH_instantiate;
		SH_ar_desc->connect_port = SH_connect_port;
		SH_ar_desc->activate = SH_activate;
		SH_ar_desc->run = SH_run_ar;
		SH_ar_desc->run_adding = NULL;
		SH_ar_desc->set_run_adding_gain = NULL;
		SH_ar_desc->deactivate = NULL;
		SH_ar_desc->cleanup = SH_cleanup;
	}
}


void
SH_delete_descriptor(LADSPA_Descriptor* psDescriptor)
{
	unsigned long lIndex;
	if (psDescriptor) {
		free((char*)psDescriptor->Label);
		free((char*)psDescriptor->Name);
		free((char*)psDescriptor->Maker);
		free((char*)psDescriptor->Copyright);
		free((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
		for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
			free((char*)(psDescriptor->PortNames[lIndex]));
		free((char**)psDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
		free(psDescriptor);
	}
}


/* Called automatically when the library is unloaded. */
void
_fini()
{
	SH_delete_descriptor(SH_cr_desc);
	SH_delete_descriptor(SH_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return SH_cr_desc;
	case 1:
		return SH_ar_desc;
	default:
		return NULL;
	}
}

