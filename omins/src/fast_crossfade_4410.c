/* Crossfade with AR Level plugin.  Copyright (C) 2005 Thorsten Wilms.
 * Based on Dave Robillard's "Hz to AMS style V/Oct" plugin for the skeleton.
 * Thanks to Florian Schmidt for explaining how to calculate the scale values 
 * before I could work it out myself! ;-)
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

#define XFADE_LEVEL_ID 4410

#define XFADE_NUM_PORTS 4

/* Port Numbers */
#define XFADE_LEVEL  0
#define XFADE_A      1
#define XFADE_B      2
#define XFADE_OUTPUT 3

/* All state information for plugin */
typedef struct {
	/* Ports */
	LADSPA_Data *level_buffer;
	LADSPA_Data *a_buffer;
	LADSPA_Data *b_buffer;
	LADSPA_Data *output_buffer;
} XFADE;


/* Construct a new plugin instance */
LADSPA_Handle
XFADE_instantiate(const LADSPA_Descriptor * descriptor, unsigned long srate)
{
	return (LADSPA_Handle)malloc(sizeof(XFADE));
}


/* Connect a port to a data location */
void
XFADE_connect_port(LADSPA_Handle instance,
				   unsigned long port, LADSPA_Data * location)
{
	XFADE* plugin;

	plugin = (XFADE*) instance;
	switch (port) {
	case XFADE_LEVEL:
		plugin->level_buffer = location;
		break;
	case XFADE_A:
		plugin->a_buffer = location;
		break;
	case XFADE_B:
		plugin->b_buffer = location;
		break;
	case XFADE_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}


void
XFADE_run_ar(LADSPA_Handle instance, unsigned long nframes)
{
	LADSPA_Data*  level;
	LADSPA_Data*  a;
	LADSPA_Data*  b;
	LADSPA_Data*  output;
	XFADE*        plugin;
	unsigned long i;
	float         l;
	
	plugin = (XFADE*)instance;

	level  = plugin->level_buffer;
	a      = plugin->a_buffer;
	b      = plugin->b_buffer;
	output = plugin->output_buffer;

	for (i = 0; i < nframes; i++) {
		/* transfer multiplication value to 0 to 1 range */
		if (level[i] < -1) {
			l = 0;
		} else if (level[i] > 1) {
			l = 1;
		} else {
			l = (level[i] + 1) / 2;
		}

		output[i] = a[i] * l + b[i] * (1 - l);
	}
}


void
XFADE_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor *xfade_descriptor = NULL;

/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

	xfade_descriptor =
		(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));

	if (xfade_descriptor) {
		xfade_descriptor->UniqueID = XFADE_LEVEL_ID;
		xfade_descriptor->Label = strdup("fast_xfade");
		xfade_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		xfade_descriptor->Name = strdup("Fast Crossfade");
		xfade_descriptor->Maker = strdup("Thorsten Wilms");
		xfade_descriptor->Copyright = strdup("GPL");
		xfade_descriptor->PortCount = XFADE_NUM_PORTS;
		port_descriptors =
			(LADSPA_PortDescriptor *) calloc(XFADE_NUM_PORTS,
											 sizeof(LADSPA_PortDescriptor));
		xfade_descriptor->PortDescriptors =
			(const LADSPA_PortDescriptor *)port_descriptors;
		port_descriptors[XFADE_LEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[XFADE_A] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[XFADE_B] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[XFADE_OUTPUT] =
			LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char **)calloc(XFADE_NUM_PORTS, sizeof(char *));
		xfade_descriptor->PortNames = (const char **)port_names;
		port_names[XFADE_LEVEL] = strdup("Level");
		port_names[XFADE_A] = strdup("A");
		port_names[XFADE_B] = strdup("B");
		port_names[XFADE_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
							calloc(XFADE_NUM_PORTS,
								   sizeof(LADSPA_PortRangeHint)));
		xfade_descriptor->PortRangeHints =
			(const LADSPA_PortRangeHint *)port_range_hints;
		port_range_hints[XFADE_LEVEL].HintDescriptor = 0;
		port_range_hints[XFADE_A].HintDescriptor = 0;
		port_range_hints[XFADE_B].HintDescriptor = 0;
		port_range_hints[XFADE_OUTPUT].HintDescriptor = 0;
		xfade_descriptor->instantiate = XFADE_instantiate;
		xfade_descriptor->connect_port = XFADE_connect_port;
		xfade_descriptor->activate = NULL;
		xfade_descriptor->run = XFADE_run_ar;
		xfade_descriptor->run_adding = NULL;
		xfade_descriptor->set_run_adding_gain = NULL;
		xfade_descriptor->deactivate = NULL;
		xfade_descriptor->cleanup = XFADE_cleanup;
	}

}


void
XFADE_delete_descriptors(LADSPA_Descriptor * psdescriptors)
{
	unsigned long lIndex;

	if (psdescriptors) {
		free((char *)psdescriptors->Label);
		free((char *)psdescriptors->Name);
		free((char *)psdescriptors->Maker);
		free((char *)psdescriptors->Copyright);
		free((LADSPA_PortDescriptor *) psdescriptors->PortDescriptors);
		for (lIndex = 0; lIndex < psdescriptors->PortCount; lIndex++)
			free((char *)(psdescriptors->PortNames[lIndex]));
		free((char **)psdescriptors->PortNames);
		free((LADSPA_PortRangeHint *) psdescriptors->PortRangeHints);
		free(psdescriptors);
	}
}


/* Called automatically when the library is unloaded. */
void
_fini()
{
	XFADE_delete_descriptors(xfade_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor *
ladspa_descriptor(unsigned long index)
{
	if (index == 0)
		return xfade_descriptor;
	return 0;
}

