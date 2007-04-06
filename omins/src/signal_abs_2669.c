/* This file is an audio plugin.  Copyright (C) 2005 Loki Davison.
 * 
 * Sign parameter is the sign of the output, 0 being negative and >0 begin positive.
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
#include <math.h>

#include "ladspa.h"

#define SIGNAL_ABS_BASE_ID 2669

#define SIGNAL_ABS_NUM_PORTS 3

/* Port Numbers */
#define SIGNAL_ABS_INPUT1   0
#define SIGNAL_ABS_SIGN  1
#define SIGNAL_ABS_OUTPUT  2


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* sign;
	LADSPA_Data* input1;
	LADSPA_Data* output;
} SignalAbs;


/* Construct a new plugin instance */
LADSPA_Handle
signalabs_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(SignalAbs));
}


/* Connect a port to a data location */
void
signalabs_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	SignalAbs* plugin;

	plugin = (SignalAbs*)instance;
	switch (port) {
	case SIGNAL_ABS_SIGN:
		plugin->sign = location;
		break;
	case SIGNAL_ABS_INPUT1:
		plugin->input1 = location;
		break;
	case SIGNAL_ABS_OUTPUT:
		plugin->output = location;
		break;
	}
}


void
signalabs_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	SignalAbs*  plugin = (SignalAbs*)instance;
	LADSPA_Data* input1  = plugin->input1;
	LADSPA_Data* output = plugin->output;
	LADSPA_Data sign = * (plugin->sign);
	size_t i;

	for (i = 0; i < nframes; ++i)
	{
    	    if(sign > 0)
	    {
		output[i] = fabs(input1[i]);
	    } 
	     else
	    {
		output[i] = fabs(input1[i]) * -1;
	    }
	}

}


void
signalabs_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	SignalAbs*  plugin  = (SignalAbs*)instance;
	LADSPA_Data* sign  = plugin->sign;
	LADSPA_Data* input1   = plugin->input1;
	LADSPA_Data* output  = plugin->output;
	size_t i;

	for (i = 0; i < nframes; ++i)
	{
    	    if(sign[i] > 0.5)
	    {
		output[i] = fabs(input1[i]);
	    } 
	     else
	    {
		output[i] = fabs(input1[i]) * -1;
	    }
	}
}


void
signalabs_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* signal_abs_cr_desc = NULL;
LADSPA_Descriptor* signal_abs_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	signal_abs_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	signal_abs_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (signal_abs_cr_desc) {

		signal_abs_cr_desc->UniqueID = SIGNAL_ABS_BASE_ID;
		signal_abs_cr_desc->Label = strdup("signal_abs_cr");
		signal_abs_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		signal_abs_cr_desc->Name = strdup("Signal Absolute value, negative or positive (CR Controls)");
		signal_abs_cr_desc->Maker = strdup("Loki Davison");
		signal_abs_cr_desc->Copyright = strdup("GPL");
		signal_abs_cr_desc->PortCount = SIGNAL_ABS_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(SIGNAL_ABS_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		signal_abs_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[SIGNAL_ABS_SIGN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[SIGNAL_ABS_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SIGNAL_ABS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(SIGNAL_ABS_NUM_PORTS, sizeof(char*));
		signal_abs_cr_desc->PortNames = (const char**)port_names;
		port_names[SIGNAL_ABS_SIGN] = strdup("Sign");
		port_names[SIGNAL_ABS_INPUT1] = strdup("Input");
		port_names[SIGNAL_ABS_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(SIGNAL_ABS_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		signal_abs_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[SIGNAL_ABS_SIGN].HintDescriptor = LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_TOGGLED;
		port_range_hints[SIGNAL_ABS_INPUT1].HintDescriptor = 0;
		port_range_hints[SIGNAL_ABS_OUTPUT].HintDescriptor = 0;
		signal_abs_cr_desc->instantiate = signalabs_instantiate;
		signal_abs_cr_desc->connect_port = signalabs_connect_port;
		signal_abs_cr_desc->activate = NULL;
		signal_abs_cr_desc->run = signalabs_run_cr;
		signal_abs_cr_desc->run_adding = NULL;
		signal_abs_cr_desc->set_run_adding_gain = NULL;
		signal_abs_cr_desc->deactivate = NULL;
		signal_abs_cr_desc->cleanup = signalabs_cleanup;
	}

	if (signal_abs_ar_desc) {

		signal_abs_ar_desc->UniqueID = SIGNAL_ABS_BASE_ID+1;
		signal_abs_ar_desc->Label = strdup("signal_abs_ar");
		signal_abs_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		signal_abs_ar_desc->Name = strdup("Signal Absolute value, negative or positive (AR Controls)");
		signal_abs_ar_desc->Maker = strdup("Loki Davison");
		signal_abs_ar_desc->Copyright = strdup("GPL");
		signal_abs_ar_desc->PortCount = SIGNAL_ABS_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(SIGNAL_ABS_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		signal_abs_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[SIGNAL_ABS_SIGN] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SIGNAL_ABS_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[SIGNAL_ABS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(SIGNAL_ABS_NUM_PORTS, sizeof(char*));
		signal_abs_ar_desc->PortNames = (const char**)port_names;
		port_names[SIGNAL_ABS_SIGN] = strdup("Sign");
		port_names[SIGNAL_ABS_INPUT1] = strdup("Input 1");
		port_names[SIGNAL_ABS_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(SIGNAL_ABS_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		signal_abs_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[SIGNAL_ABS_SIGN].HintDescriptor = 0;
		port_range_hints[SIGNAL_ABS_INPUT1].HintDescriptor = 0;
		port_range_hints[SIGNAL_ABS_OUTPUT].HintDescriptor = 0;
		signal_abs_ar_desc->instantiate = signalabs_instantiate;
		signal_abs_ar_desc->connect_port = signalabs_connect_port;
		signal_abs_ar_desc->activate = NULL;
		signal_abs_ar_desc->run = signalabs_run_ar;
		signal_abs_ar_desc->run_adding = NULL;
		signal_abs_ar_desc->set_run_adding_gain = NULL;
		signal_abs_ar_desc->deactivate = NULL;
		signal_abs_ar_desc->cleanup = signalabs_cleanup;
	}
}


void
signalabs_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	signalabs_delete_descriptor(signal_abs_cr_desc);
	signalabs_delete_descriptor(signal_abs_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return signal_abs_cr_desc;
	case 1:
		return signal_abs_ar_desc;
	default:
		return NULL;
	}
}

