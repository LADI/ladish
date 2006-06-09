/* Formant filter plugin.  Copyright (C) 2005 Dave Robillard.
 *
 * Based on SSM formant filter,
 * Copyright (C) 2001 David Griffiths <dave@pawfal.org>
 *
 * Based on public domain code from alex@smartelectronix.com
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

#define FORMANT_BASE_ID 4300

#define FORMANT_NUM_PORTS 3

/* Port Numbers */
#define FORMANT_VOWEL   0
#define FORMANT_INPUT   1
#define FORMANT_OUTPUT  2


/* Vowel Coefficients */
const double coeff[5][11] = {
	{ /* A */ 8.11044e-06,
		8.943665402, -36.83889529, 92.01697887, -154.337906, 181.6233289,
		-151.8651235, 89.09614114, -35.10298511, 8.388101016, -0.923313471
	},
	{ /* E */ 4.36215e-06,
	 8.90438318, -36.55179099, 91.05750846, -152.422234, 179.1170248,
		 -149.6496211, 87.78352223, -34.60687431, 8.282228154, -0.914150747
	},
	{ /* I */ 3.33819e-06,
	  8.893102966, -36.49532826, 90.96543286, -152.4545478, 179.4835618,
	  -150.315433, 88.43409371, -34.98612086, 8.407803364, -0.932568035
	},
	{ /* O */ 1.13572e-06,
	 8.994734087, -37.2084849, 93.22900521, -156.6929844, 184.596544,
	 -154.3755513, 90.49663749, -35.58964535, 8.478996281, -0.929252233
	},
	{ /* U */ 4.09431e-07,
	 8.997322763, -37.20218544, 93.11385476, -156.2530937, 183.7080141,
	 -153.2631681, 89.59539726, -35.12454591, 8.338655623, -0.910251753
	}
};


/* All state information for plugin */
typedef struct
{
	/* Ports */
	LADSPA_Data* vowel;
	LADSPA_Data* input;
	LADSPA_Data* output;

	double memory[5][10];
} Formant;


/* Linear interpolation */
inline float
linear(float bot, float top, float pos, float val1, float val2)
{
    float t = (pos - bot) / (top - bot);
    return val1 * t + val2 * (1.0f - t);
}


/* Construct a new plugin instance */
LADSPA_Handle
formant_instantiate(const LADSPA_Descriptor* descriptor,
                    unsigned long            srate)
{
	return (LADSPA_Handle)malloc(sizeof(Formant));
}


/** Activate an instance */
void
formant_activate(LADSPA_Handle instance)
{
	Formant* plugin = (Formant*)instance;
	int i, j;
	
	for (i = 0; i < 5; ++i)
		for (j = 0; j < 10; ++j)
			plugin->memory[i][j] = 0.0;
}


/* Connect a port to a data location */
void
formant_connect_port(LADSPA_Handle instance,
                     unsigned long port,
                     LADSPA_Data*  location)
{
	Formant* plugin;

	plugin = (Formant*)instance;
	switch (port) {
	case FORMANT_VOWEL:
		plugin->vowel = location;
		break;
	case FORMANT_INPUT:
		plugin->input = location;
		break;
	case FORMANT_OUTPUT:
		plugin->output = location;
		break;
	}
}


void
formant_run_vc(LADSPA_Handle instance, unsigned long nframes)
{
	Formant*     plugin = (Formant*)instance;
	LADSPA_Data  vowel;
	LADSPA_Data  in;
	LADSPA_Data* out;
	LADSPA_Data  res;
	LADSPA_Data  o[5];
	size_t       n, v;
	
	for (n=0; n < nframes; ++n) {
		vowel = plugin->vowel[0];
		in = plugin->input[n];
		out = plugin->output;
		
		for (v=0; v < 5; ++v) {
			res = (float) (coeff[v][0] * (in * 0.1f) +
			              coeff[v][1] * plugin->memory[v][0] +
			              coeff[v][2] * plugin->memory[v][1] +
			              coeff[v][3] * plugin->memory[v][2] +
			              coeff[v][4] * plugin->memory[v][3] +
			              coeff[v][5] * plugin->memory[v][4] +
			              coeff[v][6] * plugin->memory[v][5] +
			              coeff[v][7] * plugin->memory[v][6] +
			              coeff[v][8] * plugin->memory[v][7] +
			              coeff[v][9] * plugin->memory[v][8] +
			              coeff[v][10] * plugin->memory[v][9] );

			plugin->memory[v][9] = plugin->memory[v][8];
			plugin->memory[v][8] = plugin->memory[v][7];
			plugin->memory[v][7] = plugin->memory[v][6];
			plugin->memory[v][6] = plugin->memory[v][5];
			plugin->memory[v][5] = plugin->memory[v][4];
			plugin->memory[v][4] = plugin->memory[v][3];
			plugin->memory[v][3] = plugin->memory[v][2];
			plugin->memory[v][2] = plugin->memory[v][1];
			plugin->memory[v][1] = plugin->memory[v][0];
			plugin->memory[v][0] = (double)res;

			o[v] = res;
		}

		// Mix between formants
		if (vowel <= 0)
			out[n] = o[0];
		else if (vowel > 0 && vowel < 1)
			out[n] = linear(0.0f,  1.0f, vowel, o[1], o[0]);
		else if (vowel == 1)
			out[n] = o[1];
		else if (vowel > 1 && vowel < 2)
			out[n] = linear(0.0f,  1.0f, vowel - 1.0f, o[2], o[1]);
		else if (vowel == 2)
			out[n] = o[2];
		else if (vowel > 2 && vowel < 3)
			out[n] = linear(0.0f,  1.0f, vowel - 2.0f, o[3], o[2]);
		else if (vowel == 3)
			out[n] = o[3];
		else if (vowel > 3 && vowel < 4)
			out[n] = linear(0.0f,  1.0f, vowel - 3.0f, o[4], o[3]);
		else //if (vowel >= 4)
			out[n] = o[4];
	}
}

/*
void
formant_run_va(LADSPA_Handle instance, unsigned long nframes)
{
	LADSPA_Data*  input;
	LADSPA_Data*  output;
	Formant*      plugin = (Formant*)instance;
}*/


void
formant_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* formant_vc_desc = NULL;
//LADSPA_Descriptor* formant_va_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	formant_vc_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
	//formant_va_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (formant_vc_desc) {

		formant_vc_desc->UniqueID = FORMANT_BASE_ID;
		formant_vc_desc->Label = strdup("formant_vc");
		formant_vc_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		formant_vc_desc->Name = strdup("Formant Filter (CR vowel)");
		formant_vc_desc->Maker = strdup("Dave Robillard");
		formant_vc_desc->Copyright = strdup("GPL");
		formant_vc_desc->PortCount = FORMANT_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(FORMANT_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		formant_vc_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[FORMANT_VOWEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[FORMANT_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[FORMANT_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(FORMANT_NUM_PORTS, sizeof(char*));
		formant_vc_desc->PortNames = (const char**)port_names;
		port_names[FORMANT_VOWEL] = strdup("Vowel");
		port_names[FORMANT_INPUT] = strdup("Input");
		port_names[FORMANT_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(FORMANT_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		formant_vc_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[FORMANT_VOWEL].LowerBound = 0.0f;
		port_range_hints[FORMANT_VOWEL].UpperBound = 4.0f;
		port_range_hints[FORMANT_VOWEL].HintDescriptor =
			LADSPA_HINT_DEFAULT_0 | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[FORMANT_INPUT].HintDescriptor = 0;
		port_range_hints[FORMANT_OUTPUT].HintDescriptor = 0;
		formant_vc_desc->instantiate = formant_instantiate;
		formant_vc_desc->connect_port = formant_connect_port;
		formant_vc_desc->activate = formant_activate;
		formant_vc_desc->run = formant_run_vc;
		formant_vc_desc->run_adding = NULL;
		formant_vc_desc->set_run_adding_gain = NULL;
		formant_vc_desc->deactivate = NULL;
		formant_vc_desc->cleanup = formant_cleanup;
	}

	/*if (formant_va_desc) {

		formant_va_desc->UniqueID = FORMANT_BASE_ID+1;
		formant_va_desc->Label = strdup("formant_va");
		formant_va_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		formant_va_desc->Name = strdup("Formant Filter (AR vowel)");
		formant_va_desc->Maker = strdup("Dave Robillard");
		formant_va_desc->Copyright = strdup("GPL");
		formant_va_desc->PortCount = FORMANT_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(FORMANT_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		formant_va_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[FORMANT_VOWEL] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[FORMANT_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[FORMANT_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names = (char**)calloc(FORMANT_NUM_PORTS, sizeof(char*));
		formant_va_desc->PortNames = (const char**)port_names;
		port_names[FORMANT_VOWEL] = strdup("Vowel");
		port_names[FORMANT_INPUT] = strdup("Input");
		port_names[FORMANT_OUTPUT] = strdup("Output");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(FORMANT_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		formant_va_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[FORMANT_VOWEL].HintDescriptor = 0;
		port_range_hints[FORMANT_INPUT].HintDescriptor = 0;
		port_range_hints[FORMANT_OUTPUT].HintDescriptor = 0;
		formant_va_desc->instantiate = formant_instantiate;
		formant_va_desc->connect_port = formant_connect_port;
		formant_va_desc->activate = formant_activate;
		formant_va_desc->run = formant_run_va;
		formant_va_desc->run_adding = NULL;
		formant_va_desc->set_run_adding_gain = NULL;
		formant_va_desc->deactivate = NULL;
		formant_va_desc->cleanup = formant_cleanup;
	}*/
}


void
formant_delete_descriptor(LADSPA_Descriptor* psDescriptor)
{
	unsigned long lIndex;
	if (psDescriptor) {
		free((char*)psDescriptor->Label);
		free((char*)psDescriptor->Name);
		free((char*)psDescriptor->Maker);
		free((char*)psDescriptor->Copyright);
		free((LADSPA_PortDescriptor*)psDescriptor->PortDescriptors);
		for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
			free((char*)(psDescriptor->PortNames[lIndex]));
		free((char**)psDescriptor->PortNames);
		free((LADSPA_PortRangeHint*)psDescriptor->PortRangeHints);
		free(psDescriptor);
	}
}


/* Called automatically when the library is unloaded. */
void
_fini()
{
	formant_delete_descriptor(formant_vc_desc);
	//formant_delete_descriptor(formant_va_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return formant_vc_desc;
	//case 1:
	//	return formant_va_desc;
	default:
		return NULL;
	}
}

