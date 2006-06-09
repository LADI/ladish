/* slew_limiter - A LADSPA plugin that limits the rate of change of a
 *                signal. Increases and decreases in the signal can be
 *                limited separately.
 *
 * Copyright (C) 2005 Lars Luthman
 * LADSPA skeleton code taken from dahdsr_fexp.c by Loki Davison
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
#include <ladspa.h>
#include <math.h>

/* This defines the number of "variants", i.e. the number of separate plugins
   in this file */
#define SLIM_VARIANT_COUNT             2

/* These are the port numbers */
#define SLIM_INPUT                     0
#define SLIM_MAXRISE                   1
#define SLIM_MAXFALL                   2
#define SLIM_OUTPUT                    3


/* This is an array pointer to the descriptors of the different variants */
LADSPA_Descriptor** slim_descriptors = 0;


/* This is the data for a single instance of the plugin */
typedef struct
{
	LADSPA_Data* input;
	LADSPA_Data* maxrise;
	LADSPA_Data* maxfall;
	LADSPA_Data* reset;
	LADSPA_Data* output;
	LADSPA_Data srate;
	LADSPA_Data last_output;
}
SLim;



/* LADSPA hosts use this function to get the plugin descriptors */
const LADSPA_Descriptor* ladspa_descriptor(unsigned long index)
{
	if (index < SLIM_VARIANT_COUNT)
		return slim_descriptors[index];
	return 0;
}


/* Clean up after a plugin instance */
void cleanupSLim (LADSPA_Handle instance)
{
	free(instance);
}


/* This is called when the hosts connects a port to a buffer */
void connectPortSLim(LADSPA_Handle instance,
                     unsigned long port, LADSPA_Data* data)
{
	SLim* plugin = (SLim *)instance;

	switch (port) {
	case SLIM_INPUT:
		plugin->input = data;
		break;
	case SLIM_MAXRISE:
		plugin->maxrise = data;
		break;
	case SLIM_MAXFALL:
		plugin->maxfall = data;
		break;
	case SLIM_OUTPUT:
		plugin->output = data;
		break;
	}
}


/* The host uses this function to create an instance of a plugin */
LADSPA_Handle instantiateSLim(const LADSPA_Descriptor * descriptor,
                              unsigned long sample_rate)
{
	SLim* plugin = (SLim*)calloc(1, sizeof(SLim));
	plugin->srate = (LADSPA_Data)sample_rate;
	return (LADSPA_Handle)plugin;
}


/* This is called before the hosts starts to use the plugin instance */
void activateSLim(LADSPA_Handle instance)
{
	SLim* plugin = (SLim*)instance;
	plugin->last_output = 0;
}


/* The run function! */
void runSLim(LADSPA_Handle instance, unsigned long sample_count, int variant)
{
	SLim* plugin = (SLim*)instance;

	if (plugin->input && plugin->output) {
		unsigned long i;
		LADSPA_Data maxrise, maxfall;
		for (i = 0; i < sample_count; ++i) {

			if (plugin->maxrise && variant == 0)
				maxrise = plugin->maxrise[i];
			else if (plugin->maxrise && variant == 1)
				maxrise = plugin->maxrise[0];
			else
				maxrise = 0;

			if (plugin->maxfall && variant == 0)
				maxfall = plugin->maxfall[i];
			else if (plugin->maxfall && variant == 1)
				maxfall = plugin->maxfall[0];
			else
				maxfall = 0;

			LADSPA_Data maxinc = maxrise / plugin->srate;
			LADSPA_Data maxdec = maxfall / plugin->srate;
			LADSPA_Data increment = plugin->input[i] - plugin->last_output;
			if (increment > maxinc)
				increment = maxinc;
			else if (increment < -maxdec)
				increment = -maxdec;

			plugin->output[i] = plugin->last_output + increment;
			plugin->last_output = plugin->output[i];
		}
	}
}


void runSLim_audio(LADSPA_Handle instance, unsigned long sample_count)
{
	runSLim(instance, sample_count, 0);
}


void runSLim_control(LADSPA_Handle instance, unsigned long sample_count)
{
	runSLim(instance, sample_count, 1);
}


/* Called automagically by the dynamic linker - set up global stuff here */
void _init(void)
{
	static const unsigned long ids[] = { 2743, 2744 };
	static const char * labels[] = { "slew_limiter_ra", "slew_limiter_rc" };
	static const char * names[] = { "Slew limiter (RA)", "Slew limiter (RC)" };
	
	char** port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint* port_range_hints;
	LADSPA_Descriptor* descriptor;

	LADSPA_PortDescriptor input_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor maxrise_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor maxfall_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor output_port_descriptors[] =
	    { LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO };

	void (*run_functions[]) (LADSPA_Handle, unsigned long) = { runSLim_audio,
	        runSLim_control };

	slim_descriptors = (LADSPA_Descriptor**)calloc(SLIM_VARIANT_COUNT,
	                   sizeof(LADSPA_Descriptor));

	if (slim_descriptors) {
		int i;
		for (i = 0; i < SLIM_VARIANT_COUNT; ++i) {
			slim_descriptors[i] =
			    (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
			descriptor = slim_descriptors[i];
			if (descriptor) {
				descriptor->UniqueID = ids[i];
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = names[i];
				descriptor->Maker = "Lars Luthman <larsl@users.sourceforge.net>";
				descriptor->Copyright = "GPL";
				descriptor->PortCount = 4;

				port_descriptors =
				    (LADSPA_PortDescriptor*)calloc(4, sizeof(LADSPA_PortDescriptor));
				descriptor->PortDescriptors =
				    (const LADSPA_PortDescriptor*)port_descriptors;
				port_range_hints =
				    (LADSPA_PortRangeHint*)calloc(4, sizeof(LADSPA_PortRangeHint));
				descriptor->PortRangeHints =
				    (const LADSPA_PortRangeHint*)port_range_hints;

				port_names = (char**)calloc(9, sizeof (char*));
				descriptor->PortNames = (const char**)port_names;

				/* Parameters for Input */
				port_descriptors[SLIM_INPUT] = input_port_descriptors[i];
				port_names[SLIM_INPUT] = "Input";

				/* Parameters for Rise rate */
				port_descriptors[SLIM_MAXRISE] = maxrise_port_descriptors[i];
				port_names[SLIM_MAXRISE] = "Rise rate (1/s)";

				/* Parameters for Fall rate */
				port_descriptors[SLIM_MAXFALL] = maxfall_port_descriptors[i];
				port_names[SLIM_MAXFALL] = "Fall rate (1/s)";

				/* Parameters for Output*/
				port_descriptors[SLIM_OUTPUT] = output_port_descriptors[i];
				port_names[SLIM_OUTPUT] = "Output";

				descriptor->activate = activateSLim;
				descriptor->cleanup = cleanupSLim;
				descriptor->connect_port = connectPortSLim;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSLim;
				descriptor->run = run_functions[i];
				descriptor->run_adding = NULL;
				descriptor->set_run_adding_gain = NULL;
			}
		}
	}
}


/* Called automagically by the dynamic linker - free global stuff here */
void _fini(void)
{
	LADSPA_Descriptor* descriptor;
	int i;

	if (slim_descriptors) {
		for (i = 0; i < SLIM_VARIANT_COUNT; i++) {
			descriptor = slim_descriptors[i];
			if (descriptor) {
				free((LADSPA_PortDescriptor*)descriptor->PortDescriptors);
				free((char**)descriptor->PortNames);
				free((LADSPA_PortRangeHint*)descriptor->PortRangeHints);
				free(descriptor);
			}
		}
		free(slim_descriptors);
	}
}
