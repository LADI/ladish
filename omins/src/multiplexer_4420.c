/* Multiplxer plugin.  Copyright (C) 2005 Thorsten Wilms.
 * GATEd on Dave Robillard's "Hz to AMS style V/Oct" plugin for the skeleton.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"

#define MUX_GATE_ID 4420

#define MUX_NUM_PORTS 4

/* Port Numbers */
#define MUX_GATE     0
#define MUX_OFF     1
#define MUX_ON  2
#define MUX_OUTPUT   3


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* gate_buffer;
	LADSPA_Data* off_buffer;
	LADSPA_Data* on_buffer;
        LADSPA_Data* output_buffer;
} MUX;


/* Construct a new plugin instance */
LADSPA_Handle
MUX_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(MUX));
}


/* Connect a port to a data location */
void
MUX_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	MUX* plugin;

	plugin = (MUX*)instance;
	switch (port) {
	case MUX_GATE:
		plugin->gate_buffer = location;
		break;
        case MUX_OFF:
		plugin->off_buffer = location;
		break;
        case MUX_ON:
		plugin->on_buffer = location;
		break;
	case MUX_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}


void
MUX_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	const MUX* const plugin = (MUX*)instance;

	if (*plugin->gate_buffer <= 0)
		*plugin->output_buffer = *plugin->off_buffer;
	else
		*plugin->output_buffer = *plugin->on_buffer;
}


void
MUX_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	const MUX* const         plugin = (MUX*)instance;
	const LADSPA_Data* const gate   = plugin->gate_buffer;
	const LADSPA_Data* const off    = plugin->off_buffer;
	const LADSPA_Data* const on     = plugin->on_buffer;
	LADSPA_Data* const       output = plugin->output_buffer;
	unsigned long i;

	for (i = 0; i < nframes; i++)
		output[i] = (gate[i] <= 0) ? off[i] : on[i];
}


void
MUX_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* MUX_cr_desc = NULL;
LADSPA_Descriptor* MUX_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	MUX_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	MUX_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (MUX_cr_desc) {
		MUX_cr_desc->UniqueID = MUX_GATE_ID;
		MUX_cr_desc->Label = strdup("mux_cr");
		MUX_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		MUX_cr_desc->Name = strdup("Multiplexer (CR)");
		MUX_cr_desc->Maker = strdup("Thorsten Wilms");
		MUX_cr_desc->Copyright = strdup("GPL");
		MUX_cr_desc->PortCount = MUX_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(MUX_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		MUX_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[MUX_GATE] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
                port_descriptors[MUX_OFF] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
                port_descriptors[MUX_ON] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[MUX_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names = (char**)calloc(MUX_NUM_PORTS, sizeof(char*));
		MUX_cr_desc->PortNames = (const char**)port_names;
		port_names[MUX_GATE] = strdup("Gate");
                port_names[MUX_OFF] = strdup("Off");
                port_names[MUX_ON] = strdup("On");
		port_names[MUX_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(MUX_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		MUX_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[MUX_GATE].HintDescriptor = 0;
                port_range_hints[MUX_OFF].HintDescriptor = 0;
                port_range_hints[MUX_ON].HintDescriptor = 0;
		port_range_hints[MUX_OUTPUT].HintDescriptor = 0;
		MUX_cr_desc->instantiate = MUX_instantiate;
		MUX_cr_desc->connect_port = MUX_connect_port;
		MUX_cr_desc->activate = NULL;
		MUX_cr_desc->run = MUX_run_cr;
		MUX_cr_desc->run_adding = NULL;
		MUX_cr_desc->set_run_adding_gain = NULL;
		MUX_cr_desc->deactivate = NULL;
		MUX_cr_desc->cleanup = MUX_cleanup;
	}

	if (MUX_ar_desc) {
		MUX_ar_desc->UniqueID = MUX_GATE_ID+1;
		MUX_ar_desc->Label = strdup("mux_ar");
		MUX_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		MUX_ar_desc->Name = strdup("Multiplexer (AR)");
		MUX_ar_desc->Maker = strdup("Thorsten Wilms");
		MUX_ar_desc->Copyright = strdup("GPL");
		MUX_ar_desc->PortCount = MUX_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(MUX_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		MUX_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[MUX_GATE] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
                port_descriptors[MUX_OFF] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
                port_descriptors[MUX_ON] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[MUX_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(MUX_NUM_PORTS, sizeof(char*));
		MUX_ar_desc->PortNames = (const char**)port_names;
		port_names[MUX_GATE] = strdup("Gate");
                port_names[MUX_OFF] = strdup("Off");
                port_names[MUX_ON] = strdup("On");
		port_names[MUX_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(MUX_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		MUX_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[MUX_GATE].HintDescriptor = 0;
                port_range_hints[MUX_OFF].HintDescriptor = 0;
                port_range_hints[MUX_ON].HintDescriptor = 0;
		port_range_hints[MUX_OUTPUT].HintDescriptor = 0;
		MUX_ar_desc->instantiate = MUX_instantiate;
		MUX_ar_desc->connect_port = MUX_connect_port;
		MUX_ar_desc->activate = NULL;
		MUX_ar_desc->run = MUX_run_ar;
		MUX_ar_desc->run_adding = NULL;
		MUX_ar_desc->set_run_adding_gain = NULL;
		MUX_ar_desc->deactivate = NULL;
		MUX_ar_desc->cleanup = MUX_cleanup;
	}
}


void
MUX_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	MUX_delete_descriptor(MUX_cr_desc);
	MUX_delete_descriptor(MUX_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return MUX_cr_desc;
	case 1:
		return MUX_ar_desc;
	default:
		return NULL;
	}
}

