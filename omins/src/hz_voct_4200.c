/* Hz to AMS style V/Oct plugin.  Copyright (C) 2005 Dave Robillard.
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

#define HZVOCT_BASE_ID 4200

#define HZVOCT_NUM_PORTS 2

/* Port Numbers */
#define HZVOCT_INPUT   0
#define HZVOCT_OUTPUT  1


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* input_buffer;
	LADSPA_Data* output_buffer;
} HzVoct;


/* Construct a new plugin instance */
LADSPA_Handle
hzvoct_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	HzVoct* plugin = malloc(sizeof(HzVoct));
	plugin->input_buffer = NULL;
	plugin->output_buffer = NULL;
	return (LADSPA_Handle)plugin;
}


/* Connect a port to a data location */
void
hzvoct_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	HzVoct* plugin;

	plugin = (HzVoct*)instance;
	switch (port) {
	case HZVOCT_INPUT:
		plugin->input_buffer = location;
		break;
	case HZVOCT_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}


void
hzvoct_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	HzVoct*       plugin;
	float         log2inv;
	float         eighth = 1.0f/8.0f;
	const float   offset = 2.0313842f; // + octave, ... -1, 0, 1 ...

	plugin = (HzVoct*)instance;
	log2inv = 1.0f/logf(2.0f);
	
	*plugin->output_buffer = logf(*plugin->input_buffer * eighth) * log2inv - offset;
}


void
hzvoct_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	LADSPA_Data*  input;
	LADSPA_Data*  output;
	HzVoct*       plugin;
	unsigned long i;
	float         log2inv;
	float         eighth = 1.0f/8.0f;
	const float   offset = 5.0313842; // + octave, ... -1, 0, 1 ...

	plugin = (HzVoct*)instance;
	log2inv = 1.0f/logf(2.0);	

	input = plugin->input_buffer;
	output = plugin->output_buffer;
	
	// Inverse of the formula used in AMS's converter module (except the 1/8 part)
	for (i = 0; i < nframes; i++)
		*output++ = logf((*input++) * eighth) * log2inv - offset;
}


void
hzvoct_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* hz_voct_cr_desc = NULL;
LADSPA_Descriptor* hz_voct_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	hz_voct_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	hz_voct_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (hz_voct_cr_desc) {

		hz_voct_cr_desc->UniqueID = HZVOCT_BASE_ID;
		hz_voct_cr_desc->Label = strdup("hz_voct_cr");
		hz_voct_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		hz_voct_cr_desc->Name = strdup("Hz to V/Oct Converter (CR)");
		hz_voct_cr_desc->Maker = strdup("Dave Robillard");
		hz_voct_cr_desc->Copyright = strdup("GPL");
		hz_voct_cr_desc->PortCount = HZVOCT_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(HZVOCT_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		hz_voct_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[HZVOCT_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[HZVOCT_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names = (char**)calloc(HZVOCT_NUM_PORTS, sizeof(char*));
		hz_voct_cr_desc->PortNames = (const char**)port_names;
		port_names[HZVOCT_INPUT] = strdup("Input");
		port_names[HZVOCT_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(HZVOCT_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		hz_voct_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[HZVOCT_INPUT].HintDescriptor = 0;
		port_range_hints[HZVOCT_OUTPUT].HintDescriptor = 0;
		hz_voct_cr_desc->instantiate = hzvoct_instantiate;
		hz_voct_cr_desc->connect_port = hzvoct_connect_port;
		hz_voct_cr_desc->activate = NULL;
		hz_voct_cr_desc->run = hzvoct_run_cr;
		hz_voct_cr_desc->run_adding = NULL;
		hz_voct_cr_desc->set_run_adding_gain = NULL;
		hz_voct_cr_desc->deactivate = NULL;
		hz_voct_cr_desc->cleanup = hzvoct_cleanup;
	}

	if (hz_voct_ar_desc) {

		hz_voct_ar_desc->UniqueID = HZVOCT_BASE_ID+1;
		hz_voct_ar_desc->Label = strdup("hz_voct_ar");
		hz_voct_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		hz_voct_ar_desc->Name = strdup("Hz to V/Oct Converter (AR)");
		hz_voct_ar_desc->Maker = strdup("Dave Robillard");
		hz_voct_ar_desc->Copyright = strdup("GPL");
		hz_voct_ar_desc->PortCount = HZVOCT_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(HZVOCT_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		hz_voct_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[HZVOCT_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[HZVOCT_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(HZVOCT_NUM_PORTS, sizeof(char*));
		hz_voct_ar_desc->PortNames = (const char**)port_names;
		port_names[HZVOCT_INPUT] = strdup("Input");
		port_names[HZVOCT_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(HZVOCT_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		hz_voct_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[HZVOCT_INPUT].HintDescriptor = 0;
		port_range_hints[HZVOCT_OUTPUT].HintDescriptor = 0;
		hz_voct_ar_desc->instantiate = hzvoct_instantiate;
		hz_voct_ar_desc->connect_port = hzvoct_connect_port;
		hz_voct_ar_desc->activate = NULL;
		hz_voct_ar_desc->run = hzvoct_run_ar;
		hz_voct_ar_desc->run_adding = NULL;
		hz_voct_ar_desc->set_run_adding_gain = NULL;
		hz_voct_ar_desc->deactivate = NULL;
		hz_voct_ar_desc->cleanup = hzvoct_cleanup;
	}
}


void
hzvoct_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	hzvoct_delete_descriptor(hz_voct_cr_desc);
	hzvoct_delete_descriptor(hz_voct_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return hz_voct_cr_desc;
	case 1:
		return hz_voct_ar_desc;
	default:
		return NULL;
	}
}

