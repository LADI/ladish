/* Base to the power of Exponent plugin.  Copyright (C) 2005 Thorsten Wilms.
 * Based on Dave Robillard's "Hz to AMS style V/Oct" plugin for the skeleton, 
 * and there's not much else in here :).
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"

#define POWER_BASE_ID 4400

#define POWER_NUM_PORTS 3

/* Port Numbers */
#define POWER_BASE     0
#define POWER_EXPONENT 1
#define POWER_RESULT   2


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* base_buffer;
	LADSPA_Data* exponent_buffer;
        LADSPA_Data* result_buffer;
} POWER;


/* Construct a new plugin instance */
LADSPA_Handle
POWER_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(POWER));
}


/* Connect a port to a data location */
void
POWER_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	POWER* plugin;

	plugin = (POWER*)instance;
	switch (port) {
	case POWER_BASE:
		plugin->base_buffer = location;
		break;
        case POWER_EXPONENT:
		plugin->exponent_buffer = location;
		break;
	case POWER_RESULT:
		plugin->result_buffer = location;
		break;
	}
}


void
POWER_run_cr(LADSPA_Handle instance, unsigned long nframes)
{
	POWER* plugin = (POWER*)instance;
	
	*plugin->result_buffer = powf(*plugin->base_buffer, *plugin->exponent_buffer);
}


void
POWER_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	const POWER* const       plugin   = (POWER*)instance;
	const LADSPA_Data* const base     = plugin->base_buffer;
	const LADSPA_Data* const exponent = plugin->exponent_buffer;
	LADSPA_Data* const       result   = plugin->result_buffer;
	unsigned long i;
	
	for (i = 0; i < nframes; ++i)
		result[i] = powf(base[i], exponent[i]);
}


void
POWER_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* power_cr_desc = NULL;
LADSPA_Descriptor* power_ar_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	power_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	power_ar_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (power_cr_desc) {

		power_cr_desc->UniqueID = POWER_BASE_ID;
		power_cr_desc->Label = strdup("power_cr");
		power_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		power_cr_desc->Name = strdup("Power (CR)");
		power_cr_desc->Maker = strdup("Thorsten Wilms");
		power_cr_desc->Copyright = strdup("GPL");
		power_cr_desc->PortCount = POWER_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(POWER_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		power_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[POWER_BASE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
                port_descriptors[POWER_EXPONENT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[POWER_RESULT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names = (char**)calloc(POWER_NUM_PORTS, sizeof(char*));
		power_cr_desc->PortNames = (const char**)port_names;
		port_names[POWER_BASE] = strdup("Base");
                port_names[POWER_EXPONENT] = strdup("Exponent");
		port_names[POWER_RESULT] = strdup("Result");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(POWER_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		power_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[POWER_BASE].HintDescriptor = 0;
                port_range_hints[POWER_EXPONENT].HintDescriptor = 0;
		port_range_hints[POWER_RESULT].HintDescriptor = 0;
		power_cr_desc->instantiate = POWER_instantiate;
		power_cr_desc->connect_port = POWER_connect_port;
		power_cr_desc->activate = NULL;
		power_cr_desc->run = POWER_run_cr;
		power_cr_desc->run_adding = NULL;
		power_cr_desc->set_run_adding_gain = NULL;
		power_cr_desc->deactivate = NULL;
		power_cr_desc->cleanup = POWER_cleanup;
	}

	if (power_ar_desc) {

		power_ar_desc->UniqueID = POWER_BASE_ID+1;
		power_ar_desc->Label = strdup("power");
		power_ar_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		power_ar_desc->Name = strdup("Power (AR)");
		power_ar_desc->Maker = strdup("Thorsten Wilms");
		power_ar_desc->Copyright = strdup("GPL");
		power_ar_desc->PortCount = POWER_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(POWER_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		power_ar_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[POWER_BASE] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
                port_descriptors[POWER_EXPONENT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[POWER_RESULT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(POWER_NUM_PORTS, sizeof(char*));
		power_ar_desc->PortNames = (const char**)port_names;
		port_names[POWER_BASE] = strdup("Base");
                port_names[POWER_EXPONENT] = strdup("Exponent");
		port_names[POWER_RESULT] = strdup("Result");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(POWER_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		power_ar_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[POWER_BASE].HintDescriptor = 0;
                port_range_hints[POWER_EXPONENT].HintDescriptor = 0;
		port_range_hints[POWER_RESULT].HintDescriptor = 0;
		power_ar_desc->instantiate = POWER_instantiate;
		power_ar_desc->connect_port = POWER_connect_port;
		power_ar_desc->activate = NULL;
		power_ar_desc->run = POWER_run_ar;
		power_ar_desc->run_adding = NULL;
		power_ar_desc->set_run_adding_gain = NULL;
		power_ar_desc->deactivate = NULL;
		power_ar_desc->cleanup = POWER_cleanup;
	}
}


void
POWER_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	POWER_delete_descriptor(power_cr_desc);
	POWER_delete_descriptor(power_ar_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return power_cr_desc;
	case 1:
		return power_ar_desc;
	default:
		return NULL;
	}
}

