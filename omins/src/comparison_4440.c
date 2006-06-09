/* Comparison plugin.  Copyright (C) 2005 Thorsten Wilms.
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

#include "ladspa.h"

#define COMP_BASE_ID 4440

#define COMP_NUM_PORTS 6

/* Port Numbers */
#define COMP_A        0
#define COMP_B        1
#define COMP_LARGER   2
#define COMP_SMALLER  3
#define COMP_A_LARGER 4
#define COMP_EQUAL    5


/* All state information for plugin */
typedef struct {
	/* Ports */
	LADSPA_Data *a_buffer;
	LADSPA_Data *b_buffer;
	LADSPA_Data *larger_buffer;
	LADSPA_Data *smaller_buffer;
	LADSPA_Data *a_larger_buffer;
	LADSPA_Data *equal_buffer;
} Comp;


/* Construct a new plugin instance */
LADSPA_Handle
comp_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
	Comp* plugin = malloc(sizeof(Comp));
	plugin->a_buffer        = NULL;
	plugin->b_buffer        = NULL;
	plugin->larger_buffer   = NULL;
	plugin->smaller_buffer  = NULL;
	plugin->a_larger_buffer = NULL;
	plugin->equal_buffer    = NULL;
	return (LADSPA_Handle)plugin;
}


/* Connect a port to a data location */
void
comp_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	Comp* plugin;

	plugin = (Comp*)instance;
	switch (port) {
	case COMP_A:
		plugin->a_buffer = location;
		break;
	case COMP_B:
		plugin->b_buffer = location;
		break;
	case COMP_LARGER:
		plugin->larger_buffer = location;
		break;
	case COMP_SMALLER:
		plugin->smaller_buffer = location;
		break;
	case COMP_A_LARGER:
		plugin->a_larger_buffer = location;
		break;
	case COMP_EQUAL:
		plugin->equal_buffer = location;
		break;
	}
}


void
comp_run_ac(LADSPA_Handle instance, unsigned long nframes)
{
	Comp* const              plugin   = (Comp*)instance;
	const LADSPA_Data* const a        = plugin->a_buffer;
	const LADSPA_Data  const b        = *plugin->b_buffer;
	LADSPA_Data* const       larger   = plugin->larger_buffer;
	LADSPA_Data* const       smaller  = plugin->smaller_buffer;
	LADSPA_Data* const       a_larger = plugin->a_larger_buffer;
	LADSPA_Data* const       equal    = plugin->equal_buffer;
	unsigned long i;
	
	for (i = 0; i < nframes; i++) {
		equal[i]    = (a[i] == b) ? 1.0  : 0.0;
		larger[i]   = (a[i] > b)  ? a[i] : b;
		smaller[i]  = (a[i] < b)  ? a[i] : b;
		a_larger[i] = (a[i] > b)  ? 1.0  : 0.0;
	}
}


void
comp_run_aa(LADSPA_Handle instance, unsigned long nframes)
{
	Comp* const              plugin   = (Comp*)instance;
	const LADSPA_Data* const a        = plugin->a_buffer;
	const LADSPA_Data* const b        = plugin->b_buffer;
	LADSPA_Data* const       larger   = plugin->larger_buffer;
	LADSPA_Data* const       smaller  = plugin->smaller_buffer;
	LADSPA_Data* const       a_larger = plugin->a_larger_buffer;
	LADSPA_Data* const       equal    = plugin->equal_buffer;
	unsigned long i;
	
	for (i = 0; i < nframes; i++) {
		equal[i]    = (a[i] == b[i]) ? 1.0  : 0.0;
		larger[i]   = (a[i] > b[i])  ? a[i] : b[i];
		smaller[i]  = (a[i] < b[i])  ? a[i] : b[i];
		a_larger[i] = (a[i] > b[i])  ? 1.0  : 0.0;
	}
}


void
comp_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* comp_ac_desc = NULL;
LADSPA_Descriptor* comp_aa_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	comp_ac_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	comp_aa_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (comp_ac_desc) {

		comp_ac_desc->UniqueID = COMP_BASE_ID;
		comp_ac_desc->Label = strdup("comp_ac");
		comp_ac_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		comp_ac_desc->Name = strdup("Comparison (AC)");
		comp_ac_desc->Maker = strdup("Thorsten Wilms");
		comp_ac_desc->Copyright = strdup("GPL");
		comp_ac_desc->PortCount = COMP_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(COMP_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		comp_ac_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[COMP_A] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_B] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[COMP_LARGER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_SMALLER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_A_LARGER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_EQUAL] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(COMP_NUM_PORTS, sizeof(char*));
		comp_ac_desc->PortNames = (const char**)port_names;
		port_names[COMP_A] = strdup("A");
		port_names[COMP_B] = strdup("B");
		port_names[COMP_LARGER] = strdup("Larger");
		port_names[COMP_SMALLER] = strdup("Smaller");
		port_names[COMP_A_LARGER] = strdup("A > B");
		port_names[COMP_EQUAL] = strdup("A = B");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(COMP_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		comp_ac_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[COMP_A].HintDescriptor = 0;
		port_range_hints[COMP_B].HintDescriptor = 0;
		port_range_hints[COMP_LARGER].HintDescriptor = 0;
		port_range_hints[COMP_SMALLER].HintDescriptor = 0;
		port_range_hints[COMP_A_LARGER].HintDescriptor = 0;
		port_range_hints[COMP_EQUAL].HintDescriptor = 0;
		comp_ac_desc->instantiate = comp_instantiate;
		comp_ac_desc->connect_port = comp_connect_port;
		comp_ac_desc->activate = NULL;
		comp_ac_desc->run = comp_run_ac;
		comp_ac_desc->run_adding = NULL;
		comp_ac_desc->set_run_adding_gain = NULL;
		comp_ac_desc->deactivate = NULL;
		comp_ac_desc->cleanup = comp_cleanup;
	}

	if (comp_aa_desc) {

		comp_aa_desc->UniqueID = COMP_BASE_ID+1;
		comp_aa_desc->Label = strdup("comp_aa");
		comp_aa_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		comp_aa_desc->Name = strdup("Comparison (AA)");
		comp_aa_desc->Maker = strdup("Thorsten Wilms");
		comp_aa_desc->Copyright = strdup("GPL");
		comp_aa_desc->PortCount = COMP_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(COMP_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		comp_aa_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[COMP_A] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_B] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_LARGER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_SMALLER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_A_LARGER] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[COMP_EQUAL] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(COMP_NUM_PORTS, sizeof(char*));
		comp_aa_desc->PortNames = (const char**)port_names;
		port_names[COMP_A] = strdup("A");
		port_names[COMP_B] = strdup("B");
		port_names[COMP_LARGER] = strdup("Larger");
		port_names[COMP_SMALLER] = strdup("Smaller");
		port_names[COMP_A_LARGER] = strdup("A > B");
		port_names[COMP_EQUAL] = strdup("A = B");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(COMP_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		comp_aa_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[COMP_A].HintDescriptor = 0;
		port_range_hints[COMP_B].HintDescriptor = 0;
		port_range_hints[COMP_LARGER].HintDescriptor = 0;
		port_range_hints[COMP_SMALLER].HintDescriptor = 0;
		port_range_hints[COMP_A_LARGER].HintDescriptor = 0;
		port_range_hints[COMP_EQUAL].HintDescriptor = 0;
		comp_aa_desc->instantiate = comp_instantiate;
		comp_aa_desc->connect_port = comp_connect_port;
		comp_aa_desc->activate = NULL;
		comp_aa_desc->run = comp_run_aa;
		comp_aa_desc->run_adding = NULL;
		comp_aa_desc->set_run_adding_gain = NULL;
		comp_aa_desc->deactivate = NULL;
		comp_aa_desc->cleanup = comp_cleanup;
	}
}


void
comp_delete_descriptor(LADSPA_Descriptor* psDescriptor)
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
	comp_delete_descriptor(comp_ac_desc);
	comp_delete_descriptor(comp_aa_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return comp_ac_desc;
	case 1:
		return comp_aa_desc;
	default:
		return NULL;
	}
}

