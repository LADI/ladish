/* This file is an audio plugin.  Copyright (C) 2005 Dave Robillard.
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

#define RANGETRANS_BASE_ID 4210

#define RANGETRANS_NUM_PORTS 6

/* Port Numbers */
#define RANGETRANS_IN_MIN  0
#define RANGETRANS_IN_MAX  1
#define RANGETRANS_OUT_MIN 2
#define RANGETRANS_OUT_MAX 3
#define RANGETRANS_INPUT   4
#define RANGETRANS_OUTPUT  5


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* in_min;
	LADSPA_Data* in_max;
	LADSPA_Data* out_min;
	LADSPA_Data* out_max;
	LADSPA_Data* input;
	LADSPA_Data* output;
} RangeTrans;


/* Construct a new plugin instance */
LADSPA_Handle
rangetrans_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(RangeTrans));
}


/* Connect a port to a data location */
void
rangetrans_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	RangeTrans* plugin;

	plugin = (RangeTrans*)instance;
	switch (port) {
	case RANGETRANS_IN_MIN:
		plugin->in_min = location;
		break;
	case RANGETRANS_IN_MAX:
		plugin->in_max = location;
		break;
	case RANGETRANS_OUT_MIN:
		plugin->out_min = location;
		break;
	case RANGETRANS_OUT_MAX:
		plugin->out_max = location;
		break;
	case RANGETRANS_INPUT:
		plugin->input = location;
		break;
	case RANGETRANS_OUTPUT:
		plugin->output = location;
		break;
	}
}


void
rangetrans_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	const RangeTrans* const  plugin  = (RangeTrans*)instance;
	const LADSPA_Data        in_min  = *plugin->in_min;
	const LADSPA_Data        in_max  = *plugin->in_max;
	const LADSPA_Data        out_min = *plugin->out_min;
	const LADSPA_Data        out_max = *plugin->out_max;
	const LADSPA_Data* const input   = plugin->input;
	LADSPA_Data* const       output  = plugin->output;
	unsigned long i;

	for (i = 0; i < nframes; ++i)
		output[i] = ((input[i] - in_min) / (in_max - in_min))
			* (out_max - out_min) + out_min;
}


void
rangetrans_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	const RangeTrans* const  plugin  = (RangeTrans*)instance;
	const LADSPA_Data* const in_min  = plugin->in_min;
	const LADSPA_Data* const in_max  = plugin->in_max;
	const LADSPA_Data* const out_min = plugin->out_min;
	const LADSPA_Data* const out_max = plugin->out_max;
	const LADSPA_Data* const input   = plugin->input;
	LADSPA_Data* const       output  = plugin->output;
	unsigned long i;

	for (i = 0; i < nframes; ++i)
		output[i] = ((input[i] - in_min[i]) / (in_max[i] - in_min[i]))
			* (out_max[i] - out_min[i]) + out_min[i];
}


void
rangetrans_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* range_trans_cr_desc = NULL;
LADSPA_Descriptor* range_trans_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	range_trans_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	range_trans_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (range_trans_cr_desc) {

		range_trans_cr_desc->UniqueID = RANGETRANS_BASE_ID;
		range_trans_cr_desc->Label = strdup("range_trans_cr");
		range_trans_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		range_trans_cr_desc->Name = strdup("Range Translator (CR Controls)");
		range_trans_cr_desc->Maker = strdup("Dave Robillard");
		range_trans_cr_desc->Copyright = strdup("GPL");
		range_trans_cr_desc->PortCount = RANGETRANS_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(RANGETRANS_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		range_trans_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[RANGETRANS_IN_MIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[RANGETRANS_IN_MAX] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[RANGETRANS_OUT_MIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[RANGETRANS_OUT_MAX] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[RANGETRANS_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(RANGETRANS_NUM_PORTS, sizeof(char*));
		range_trans_cr_desc->PortNames = (const char**)port_names;
		port_names[RANGETRANS_IN_MIN] = strdup("Input Min");
		port_names[RANGETRANS_IN_MAX] = strdup("Input Max");
		port_names[RANGETRANS_OUT_MIN] = strdup("Output Min");
		port_names[RANGETRANS_OUT_MAX] = strdup("Output Max");
		port_names[RANGETRANS_INPUT] = strdup("Input");
		port_names[RANGETRANS_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(RANGETRANS_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		range_trans_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[RANGETRANS_IN_MIN].HintDescriptor = LADSPA_HINT_DEFAULT_0;
		port_range_hints[RANGETRANS_IN_MAX].HintDescriptor = LADSPA_HINT_DEFAULT_1;
		port_range_hints[RANGETRANS_OUT_MIN].HintDescriptor = LADSPA_HINT_DEFAULT_0;
		port_range_hints[RANGETRANS_OUT_MAX].HintDescriptor = LADSPA_HINT_DEFAULT_1;
		port_range_hints[RANGETRANS_INPUT].HintDescriptor = 0;
		port_range_hints[RANGETRANS_OUTPUT].HintDescriptor = 0;
		range_trans_cr_desc->instantiate = rangetrans_instantiate;
		range_trans_cr_desc->connect_port = rangetrans_connect_port;
		range_trans_cr_desc->activate = NULL;
		range_trans_cr_desc->run = rangetrans_run_cr;
		range_trans_cr_desc->run_adding = NULL;
		range_trans_cr_desc->set_run_adding_gain = NULL;
		range_trans_cr_desc->deactivate = NULL;
		range_trans_cr_desc->cleanup = rangetrans_cleanup;
	}

	if (range_trans_ar_desc) {

		range_trans_ar_desc->UniqueID = RANGETRANS_BASE_ID+1;
		range_trans_ar_desc->Label = strdup("range_trans_ar");
		range_trans_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		range_trans_ar_desc->Name = strdup("Range Translator (AR Controls)");
		range_trans_ar_desc->Maker = strdup("Dave Robillard");
		range_trans_ar_desc->Copyright = strdup("GPL");
		range_trans_ar_desc->PortCount = RANGETRANS_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(RANGETRANS_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		range_trans_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[RANGETRANS_IN_MIN] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_IN_MAX] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_OUT_MIN] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_OUT_MAX] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[RANGETRANS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(RANGETRANS_NUM_PORTS, sizeof(char*));
		range_trans_ar_desc->PortNames = (const char**)port_names;
		port_names[RANGETRANS_IN_MIN] = strdup("Input Min");
		port_names[RANGETRANS_IN_MAX] = strdup("Input Max");
		port_names[RANGETRANS_OUT_MIN] = strdup("Output Min");
		port_names[RANGETRANS_OUT_MAX] = strdup("Output Max");
		port_names[RANGETRANS_INPUT] = strdup("Input");
		port_names[RANGETRANS_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(RANGETRANS_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		range_trans_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[RANGETRANS_IN_MIN].HintDescriptor = 0;
		port_range_hints[RANGETRANS_IN_MAX].HintDescriptor = 0;
		port_range_hints[RANGETRANS_OUT_MIN].HintDescriptor = 0;
		port_range_hints[RANGETRANS_OUT_MAX].HintDescriptor = 0;
		port_range_hints[RANGETRANS_INPUT].HintDescriptor = 0;
		port_range_hints[RANGETRANS_OUTPUT].HintDescriptor = 0;
		range_trans_ar_desc->instantiate = rangetrans_instantiate;
		range_trans_ar_desc->connect_port = rangetrans_connect_port;
		range_trans_ar_desc->activate = NULL;
		range_trans_ar_desc->run = rangetrans_run_ar;
		range_trans_ar_desc->run_adding = NULL;
		range_trans_ar_desc->set_run_adding_gain = NULL;
		range_trans_ar_desc->deactivate = NULL;
		range_trans_ar_desc->cleanup = rangetrans_cleanup;
	}
}


void
rangetrans_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	rangetrans_delete_descriptor(range_trans_cr_desc);
	rangetrans_delete_descriptor(range_trans_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return range_trans_cr_desc;
	case 1:
		return range_trans_ar_desc;
	default:
		return NULL;
	}
}

