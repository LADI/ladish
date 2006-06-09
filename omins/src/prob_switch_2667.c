/* This file is an audio plugin.  Copyright (C) 2005 Loki Davison.
 * 
 * Probability parameter is the prob of input 1 being the output value. 
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

#define PROBSWITCH_BASE_ID 2667

#define PROBSWITCH_NUM_PORTS 4

/* Port Numbers */
#define PROBSWITCH_INPUT1   0
#define PROBSWITCH_INPUT2  1
#define PROBSWITCH_PROB  2
#define PROBSWITCH_OUTPUT  3


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* input2;
	LADSPA_Data* prob;
	LADSPA_Data* input1;
	LADSPA_Data* output;
} ProbSwitch;


/* Construct a new plugin instance */
LADSPA_Handle
probswitch_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(ProbSwitch));
}


/* Connect a port to a data location */
void
probswitch_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	ProbSwitch* plugin;

	plugin = (ProbSwitch*)instance;
	switch (port) {
	case PROBSWITCH_INPUT2:
		plugin->input2 = location;
		break;
	case PROBSWITCH_PROB:
		plugin->prob = location;
		break;
	case PROBSWITCH_INPUT1:
		plugin->input1 = location;
		break;
	case PROBSWITCH_OUTPUT:
		plugin->output = location;
		break;
	}
}


void
probswitch_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	ProbSwitch*  plugin = (ProbSwitch*)instance;
	LADSPA_Data* input1  = plugin->input1;
	LADSPA_Data* input2  = plugin->input2;
	LADSPA_Data* output = plugin->output;
	LADSPA_Data prob = * (plugin->prob);
	size_t i;
	LADSPA_Data temp;

	for (i = 0; i < nframes; ++i)
	{
	    temp = rand();
    	    if((temp/RAND_MAX) <= prob)
	    {
		output[i] = input1[i];
	    } 
	     else
	    {
		output[i] = input2[i];
	    }
	}

}


void
probswitch_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	ProbSwitch*  plugin  = (ProbSwitch*)instance;
	LADSPA_Data* input2  = plugin->input2;
	LADSPA_Data* prob  = plugin->prob;
	LADSPA_Data* input1   = plugin->input1;
	LADSPA_Data* output  = plugin->output;
	size_t i;

	for (i = 0; i < nframes; ++i)
	{
    	    if((rand()/RAND_MAX) <= prob[i])
	    {
		output[i] = input1[i];
	    } 
	     else
	    {
		output[i] = input2[i];
	    }
	}
}


void
probswitch_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* prob_switch_cr_desc = NULL;
LADSPA_Descriptor* prob_switch_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	prob_switch_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	prob_switch_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (prob_switch_cr_desc) {

		prob_switch_cr_desc->UniqueID = PROBSWITCH_BASE_ID;
		prob_switch_cr_desc->Label = strdup("prob_switch_cr");
		prob_switch_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		prob_switch_cr_desc->Name = strdup("Probability Switch (CR Controls)");
		prob_switch_cr_desc->Maker = strdup("Loki Davison");
		prob_switch_cr_desc->Copyright = strdup("GPL");
		prob_switch_cr_desc->PortCount = PROBSWITCH_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(PROBSWITCH_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		prob_switch_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[PROBSWITCH_INPUT2] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[PROBSWITCH_PROB] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[PROBSWITCH_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[PROBSWITCH_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(PROBSWITCH_NUM_PORTS, sizeof(char*));
		prob_switch_cr_desc->PortNames = (const char**)port_names;
		port_names[PROBSWITCH_INPUT2] = strdup("Input 2");
		port_names[PROBSWITCH_PROB] = strdup("Probability");
		port_names[PROBSWITCH_INPUT1] = strdup("Input 1");
		port_names[PROBSWITCH_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(PROBSWITCH_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		prob_switch_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[PROBSWITCH_INPUT2].HintDescriptor = 0;
		port_range_hints[PROBSWITCH_PROB].HintDescriptor = LADSPA_HINT_DEFAULT_1;
		port_range_hints[PROBSWITCH_INPUT1].HintDescriptor = 0;
		port_range_hints[PROBSWITCH_OUTPUT].HintDescriptor = 0;
		prob_switch_cr_desc->instantiate = probswitch_instantiate;
		prob_switch_cr_desc->connect_port = probswitch_connect_port;
		prob_switch_cr_desc->activate = NULL;
		prob_switch_cr_desc->run = probswitch_run_cr;
		prob_switch_cr_desc->run_adding = NULL;
		prob_switch_cr_desc->set_run_adding_gain = NULL;
		prob_switch_cr_desc->deactivate = NULL;
		prob_switch_cr_desc->cleanup = probswitch_cleanup;
	}

	if (prob_switch_ar_desc) {

		prob_switch_ar_desc->UniqueID = PROBSWITCH_BASE_ID+1;
		prob_switch_ar_desc->Label = strdup("prob_switch_ar");
		prob_switch_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		prob_switch_ar_desc->Name = strdup("Probability Switch (AR Controls)");
		prob_switch_ar_desc->Maker = strdup("Loki Davison");
		prob_switch_ar_desc->Copyright = strdup("GPL");
		prob_switch_ar_desc->PortCount = PROBSWITCH_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(PROBSWITCH_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		prob_switch_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[PROBSWITCH_INPUT2] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[PROBSWITCH_PROB] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[PROBSWITCH_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[PROBSWITCH_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(PROBSWITCH_NUM_PORTS, sizeof(char*));
		prob_switch_ar_desc->PortNames = (const char**)port_names;
		port_names[PROBSWITCH_INPUT2] = strdup("Input 2");
		port_names[PROBSWITCH_PROB] = strdup("Probability");
		port_names[PROBSWITCH_INPUT1] = strdup("Input 1");
		port_names[PROBSWITCH_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(PROBSWITCH_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		prob_switch_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[PROBSWITCH_INPUT2].HintDescriptor = 0;
		port_range_hints[PROBSWITCH_PROB].HintDescriptor = 0;
		port_range_hints[PROBSWITCH_INPUT1].HintDescriptor = 0;
		port_range_hints[PROBSWITCH_OUTPUT].HintDescriptor = 0;
		prob_switch_ar_desc->instantiate = probswitch_instantiate;
		prob_switch_ar_desc->connect_port = probswitch_connect_port;
		prob_switch_ar_desc->activate = NULL;
		prob_switch_ar_desc->run = probswitch_run_ar;
		prob_switch_ar_desc->run_adding = NULL;
		prob_switch_ar_desc->set_run_adding_gain = NULL;
		prob_switch_ar_desc->deactivate = NULL;
		prob_switch_ar_desc->cleanup = probswitch_cleanup;
	}
}


void
probswitch_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	probswitch_delete_descriptor(prob_switch_cr_desc);
	probswitch_delete_descriptor(prob_switch_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return prob_switch_cr_desc;
	case 1:
		return prob_switch_ar_desc;
	default:
		return NULL;
	}
}

