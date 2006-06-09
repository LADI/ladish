/* slide.c - A LADSPA plugin that "slides" the output signal between
 *           the current and the previous input value, taking the given
 *           number of seconds to reach the current input value.
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
   in this library */
#define SLIDE_VARIANT_COUNT             2

/* These are the port numbers */
#define SLIDE_INPUT                     0
#define SLIDE_RISETIME                  1
#define SLIDE_FALLTIME                  2
#define SLIDE_OUTPUT                    3


/* This is an array pointer to the descriptors of the different variants */
LADSPA_Descriptor** slide_descriptors = 0;


/* This is the data for a single instance of the plugin */
typedef struct
{
	LADSPA_Data* input;
	LADSPA_Data* risetime;
	LADSPA_Data* falltime;
	LADSPA_Data* output;
	LADSPA_Data srate;
	LADSPA_Data from;
	LADSPA_Data to;
	LADSPA_Data last_output;
} Slide;



/* LADSPA hosts use this function to get the plugin descriptors */
const LADSPA_Descriptor* ladspa_descriptor(unsigned long index)
{
	if (index < SLIDE_VARIANT_COUNT)
		return slide_descriptors[index];
	return 0;
}


/* Clean up after a plugin instance */
void cleanupSlide (LADSPA_Handle instance)
{
	free(instance);
}


/* This is called when the hosts connects a port to a buffer */
void connectPortSlide(LADSPA_Handle instance,
                      unsigned long port, LADSPA_Data* data)
{
	Slide* plugin = (Slide *)instance;

	switch (port) {
	case SLIDE_INPUT:
		plugin->input = data;
		break;
	case SLIDE_RISETIME:
		plugin->risetime = data;
		break;
	case SLIDE_FALLTIME:
		plugin->falltime = data;
		break;
	case SLIDE_OUTPUT:
		plugin->output = data;
		break;
	}
}


/* The host uses this function to create an instance of a plugin */
LADSPA_Handle instantiateSlide(const LADSPA_Descriptor * descriptor,
                               unsigned long sample_rate)
{
	Slide* plugin = (Slide*)calloc(1, sizeof(Slide));
	plugin->srate = (LADSPA_Data)sample_rate;
	return (LADSPA_Handle)plugin;
}


/* This is called before the hosts starts to use the plugin instance */
void activateSlide(LADSPA_Handle instance)
{
	Slide* plugin = (Slide*)instance;
	plugin->last_output = 0;
	plugin->from = 0;
	plugin->to = 0;
}


/* The run function! */
void runSlide(LADSPA_Handle instance, unsigned long sample_count, int variant)
{
	Slide* plugin = (Slide*)instance;

	if (plugin->input && plugin->output) {
		unsigned long i;
		LADSPA_Data risetime, falltime;
		for (i = 0; i < sample_count; ++i) {

			if (plugin->risetime && variant == 0)
				risetime = plugin->risetime[i];
			else if (plugin->risetime && variant == 1)
				risetime = plugin->risetime[0];
			else
				risetime = 0;

			if (plugin->falltime)
				falltime = plugin->falltime[i];
			else if (plugin->falltime && variant == 1)
				falltime = plugin->falltime[0];
			else
				falltime = 0;

			/* If the signal changed, start sliding to the new value */
			if (plugin->input[i] != plugin->to) {
				plugin->from = plugin->last_output;
				plugin->to = plugin->input[i];
			}

			LADSPA_Data time;
			int rising;
			if (plugin->to > plugin->from) {
				time = risetime;
				rising = 1;
			} else {
				time = falltime;
				rising = 0;
			}

			/* If the rise/fall time is 0, just copy the input to the output */
			if (time == 0)
				plugin->output[i] = plugin->input[i];

			/* Otherwise, do the portamento */
			else {
				LADSPA_Data increment =
				    (plugin->to - plugin->from) / (time * plugin->srate);
				plugin->output[i] = plugin->last_output + increment;
				if ((plugin->output[i] > plugin->to && rising) ||
				        (plugin->output[i] < plugin->to && !rising)) {
					plugin->output[i] = plugin->to;
				}
			}

			plugin->last_output = plugin->output[i];
		}
	}
}


void runSlide_audio(LADSPA_Handle instance, unsigned long sample_count)
{
	runSlide(instance, sample_count, 0);
}


void runSlide_control(LADSPA_Handle instance, unsigned long sample_count)
{
	runSlide(instance, sample_count, 1);
}


/* Called automagically by the dynamic linker - set up global stuff here */
void _init(void)
{
	static const unsigned long ids[] = { 2741, 2742 };
	static const char * labels[] = { "slide_ta", "slide_tc" };
	static const char * names[] = { "Slide (TA)", "Slide (TC)" };

	char** port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint* port_range_hints;
	LADSPA_Descriptor* descriptor;

	LADSPA_PortDescriptor input_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor risetime_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor falltime_port_descriptors[] =
	    { LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor output_port_descriptors[] =
	    { LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	      LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO };

	void (*run_functions[])(LADSPA_Handle, unsigned long) =
	    { runSlide_audio, runSlide_control };

	slide_descriptors = (LADSPA_Descriptor**)calloc(SLIDE_VARIANT_COUNT,
	                    sizeof(LADSPA_Descriptor));

	if (slide_descriptors) {
		int i;
		for (i = 0; i < SLIDE_VARIANT_COUNT; ++i) {
			slide_descriptors[i] =
			    (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));
			descriptor = slide_descriptors[i];
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

				port_names = (char **) calloc (9, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

				/* Parameters for Input */
				port_descriptors[SLIDE_INPUT] = input_port_descriptors[i];
				port_names[SLIDE_INPUT] = "Input";

				/* Parameters for Rise rate */
				port_descriptors[SLIDE_RISETIME] = risetime_port_descriptors[i];
				port_names[SLIDE_RISETIME] = "Rise time (s)";

				/* Parameters for Fall rate */
				port_descriptors[SLIDE_FALLTIME] = falltime_port_descriptors[i];
				port_names[SLIDE_FALLTIME] = "Fall time (s)";

				/* Parameters for Output*/
				port_descriptors[SLIDE_OUTPUT] = output_port_descriptors[i];
				port_names[SLIDE_OUTPUT] = "Output";

				descriptor->activate = activateSlide;
				descriptor->cleanup = cleanupSlide;
				descriptor->connect_port = connectPortSlide;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSlide;
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

	if (slide_descriptors) {
		for (i = 0; i < SLIDE_VARIANT_COUNT; i++) {
			descriptor = slide_descriptors[i];
			if (descriptor) {
				free((LADSPA_PortDescriptor*)descriptor->PortDescriptors);
				free((char**)descriptor->PortNames);
				free((LADSPA_PortRangeHint*)descriptor->PortRangeHints);
				free(descriptor);
			}
		}
		free(slide_descriptors);
	}
}

