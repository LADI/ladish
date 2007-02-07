/*  Masher
 *  Copyright (C) 2001 David Griffiths <dave@pawfal.org>
 *  LADSPAfication (C) 2005 Dave Robillard <drobilla@connect.carelton.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */


/* NOTE:  This is a very dirty hack full of arbitrary limits and assumptions.
 * It needs fixing/completion */


#define _XOPEN_SOURCE 600 /* posix_memalign */
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "ladspa.h"

#define MASHER_BASE_ID 4310

#define MASHER_NUM_PORTS 4

/* Port Numbers */
#define MASHER_INPUT      0
#define MASHER_GRAINPITCH 1
#define MASHER_DENSITY    2
#define MASHER_OUTPUT     3

#define GRAINSTORE_SIZE 1000
#define OVERLAPS_SIZE 1000
#define MAX_GRAIN_SIZE 2048

typedef struct {
	LADSPA_Data* data;
	size_t       length;
} Sample;


typedef struct {
	int pos;
	int grain;
} GrainDesc;


/* All state information for plugin */
typedef struct {
	/* Ports */
	LADSPA_Data *input;
	LADSPA_Data *grain_pitch;
	LADSPA_Data *density;
	LADSPA_Data *output;
	
	Sample grain_store[GRAINSTORE_SIZE];
	GrainDesc overlaps[OVERLAPS_SIZE];
	size_t overlaps_size;

	size_t write_grain;
} Masher;


float
rand_range(float l, float h)
{
	return ((rand() % 10000 / 10000.0f) * (h - l)) + l;
}


/* Construct a new plugin instance */
LADSPA_Handle
masher_instantiate(const LADSPA_Descriptor * descriptor, unsigned long srate)
{
	return (LADSPA_Handle)malloc(sizeof(Masher));
}


/** Activate an instance */
void
masher_activate(LADSPA_Handle instance)
{
	Masher *plugin = (Masher*)instance;
	int i = 0;
	
	plugin->overlaps_size = 0;
	plugin->write_grain = 0;

	for (i=0; i < GRAINSTORE_SIZE; ++i) {
		//plugin->grain_store[i].data = (LADSPA_Data*)calloc(MAX_GRAIN_SIZE, sizeof(LADSPA_Data));
		posix_memalign((void**)&plugin->grain_store[i].data, 16, MAX_GRAIN_SIZE * sizeof(LADSPA_Data));
		plugin->grain_store[i].length = 0;
	}
}


/* Connect a port to a data location */
void
masher_connect_port(LADSPA_Handle instance,
					unsigned long port, LADSPA_Data * location)
{
	Masher *plugin = (Masher *) instance;

	switch (port) {
	case MASHER_INPUT:
		plugin->input = location;
		break;
	case MASHER_GRAINPITCH:
		plugin->grain_pitch = location;
		break;
	case MASHER_DENSITY:
		plugin->density = location;
		break;
	case MASHER_OUTPUT:
		plugin->output = location;
		break;
	}
}


void
mix_pitch(Sample* src, Sample* dst, size_t pos, float pitch)
{
	float  n = 0;
	size_t p = pos;

	while (n < src->length && p < dst->length) {
		dst->data[p] = dst->data[p] + src->data[(size_t)n];
		n += pitch;
		p++;
	}
}


void
masher_run(LADSPA_Handle instance, unsigned long nframes)
{
	Masher* plugin = (Masher*)instance;
	
	static const int randomness       = 1.0; // FIXME: make a control port
	int              read_grain       = 0;   // FIXME: what is this?
	int              grain_store_size = 100; // FIXME: what is this? (max 1000)

	const LADSPA_Data grain_pitch = *plugin->grain_pitch;
	const LADSPA_Data density     = *plugin->density;
	
	const LADSPA_Data* const in  = plugin->input;
	LADSPA_Data* const       out = plugin->output;
	
	Sample out_sample = { out, nframes };

	size_t n           = 0;
	float  s           = in[0];
	int    last        = 0;
	bool   first       = true;
	size_t grain_index = 0;
	size_t next_grain  = 0;
	
	// Zero output buffer
	for (n = 0; n < nframes; ++n)
		out[n] = 0.0f;

	// Paste any overlapping grains to the start of the buffer.
	for (n = 0; n < plugin->overlaps_size; ++n) {
		mix_pitch(&plugin->grain_store[plugin->overlaps[n].grain], &out_sample,
				plugin->overlaps[n].pos - nframes, grain_pitch);
	}
	plugin->overlaps_size = 0;

	// Chop up the buffer and put the grains in the grainstore
	for (n = 0; n < nframes; n++) {
		if ((s < 0 && in[n] > 0) || (s > 0 && in[n] < 0)) {
			// Chop the bits between zero crossings
			if (!first) {
				if (n - last <= MAX_GRAIN_SIZE) {
					grain_index = plugin->write_grain % grain_store_size;
					memcpy(plugin->grain_store[grain_index].data, in, nframes);
					plugin->grain_store[grain_index].length = n - last;
				}
				plugin->write_grain++; // FIXME: overflow?
			} else {
				first = false;
			}

			last = n;
			s = in[n];
		}
	}

	for (n = 0; n < nframes; n++) {
		if (n >= next_grain || rand() % 1000 < density) {
			size_t grain_num = read_grain % grain_store_size;
			mix_pitch(&plugin->grain_store[grain_num], &out_sample, n, grain_pitch);
			size_t grain_length = (plugin->grain_store[grain_num].length * grain_pitch);

			next_grain = n + plugin->grain_store[grain_num].length;

			// If this grain overlaps the buffer
			if (n + grain_length > nframes) {
				if (plugin->overlaps_size < OVERLAPS_SIZE) {
					GrainDesc new_grain;
	
					new_grain.pos = n;
					new_grain.grain = grain_num;
					plugin->overlaps[plugin->overlaps_size++] = new_grain;
				}
			}

			if (randomness)
				read_grain += 1 + rand() % randomness;
			else
				read_grain++;
		}
	}
}


void
masher_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor *masher_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

	masher_desc = (LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));

	if (masher_desc) {

		masher_desc->UniqueID = MASHER_BASE_ID;
		masher_desc->Label = strdup("ssm_masher");
		masher_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		masher_desc->Name = strdup("Masher");
		masher_desc->Maker = strdup("Dave Griffiths");
		masher_desc->Copyright = strdup("GPL");
		masher_desc->PortCount = MASHER_NUM_PORTS;
		port_descriptors =
			(LADSPA_PortDescriptor *) calloc(MASHER_NUM_PORTS,
											 sizeof(LADSPA_PortDescriptor));
		masher_desc->PortDescriptors =
			(const LADSPA_PortDescriptor *)port_descriptors;
		port_descriptors[MASHER_INPUT] =
			LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[MASHER_GRAINPITCH] =
			LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[MASHER_DENSITY] =
			LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[MASHER_OUTPUT] =
			LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char **)calloc(MASHER_NUM_PORTS, sizeof(char *));
		masher_desc->PortNames = (const char **)port_names;
		port_names[MASHER_INPUT] = strdup("Input");
		port_names[MASHER_GRAINPITCH] = strdup("Grain Pitch");
		port_names[MASHER_DENSITY] = strdup("Density");
		port_names[MASHER_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
							calloc(MASHER_NUM_PORTS,
								   sizeof(LADSPA_PortRangeHint)));
		masher_desc->PortRangeHints =
			(const LADSPA_PortRangeHint *)port_range_hints;

		port_range_hints[MASHER_GRAINPITCH].LowerBound = 1.0f;
		port_range_hints[MASHER_GRAINPITCH].UpperBound = 10.0f;
		port_range_hints[MASHER_GRAINPITCH].HintDescriptor =
			LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_BOUNDED_BELOW |
			LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[MASHER_DENSITY].LowerBound = 0.0f;
		port_range_hints[MASHER_DENSITY].UpperBound = 800.0f;
		port_range_hints[MASHER_DENSITY].HintDescriptor =
			LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_BOUNDED_BELOW |
			LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[MASHER_INPUT].HintDescriptor = 0;
		port_range_hints[MASHER_OUTPUT].HintDescriptor = 0;
		masher_desc->instantiate = masher_instantiate;
		masher_desc->connect_port = masher_connect_port;
		masher_desc->activate = masher_activate;
		masher_desc->run = masher_run;
		masher_desc->run_adding = NULL;
		masher_desc->set_run_adding_gain = NULL;
		masher_desc->deactivate = NULL;
		masher_desc->cleanup = masher_cleanup;
	}
}


void
masher_delete_descriptor(LADSPA_Descriptor * psDescriptor)
{
	unsigned long lIndex;

	if (psDescriptor) {
		free((char *)psDescriptor->Label);
		free((char *)psDescriptor->Name);
		free((char *)psDescriptor->Maker);
		free((char *)psDescriptor->Copyright);
		free((LADSPA_PortDescriptor *) psDescriptor->PortDescriptors);
		for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
			free((char *)(psDescriptor->PortNames[lIndex]));
		free((char **)psDescriptor->PortNames);
		free((LADSPA_PortRangeHint *) psDescriptor->PortRangeHints);
		free(psDescriptor);
	}
}

/* Called automatically when the library is unloaded. */
void
_fini()
{
	masher_delete_descriptor(masher_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor *
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return masher_desc;
	default:
		return NULL;
	}
}


