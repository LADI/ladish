/*
    adenv.c - A LADSPA plugin to generate percussive (i.e no sustain time), linear AD envelopes.
                 
    Copyright (C) 2005 Loki Davison 
    based on ADENV by Mike Rawes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define _XOPEN_SOURCE 500		/* strdup */
#include <stdlib.h>
#include <ladspa.h>
#include <stdio.h>
#include <math.h>

#ifdef ENABLE_NLS
#  include <locale.h>
#  define G_(s) gettext(s)
#else
#  define G_(s) (s)
#endif
#define G_NOP(s) s


#define ADENV_BASE_ID                   2661
#define ADENV_VARIANT_COUNT             1

#define ADENV_GATE                      0
#define ADENV_TRIGGER                   1
#define ADENV_ATTACK                    2
#define ADENV_DECAY                     3
#define ADENV_OUTPUT                    4

LADSPA_Descriptor **dahdsr_descriptors = 0;

typedef enum {
	IDLE,
	ATTACK,
	DECAY,
} ADENVState;

typedef struct {
	LADSPA_Data *gate;
	LADSPA_Data *trigger;
	LADSPA_Data *attack;
	LADSPA_Data *decay;
	LADSPA_Data *output;
	LADSPA_Data srate;
	LADSPA_Data inv_srate;
	LADSPA_Data last_gate;
	LADSPA_Data last_trigger;
	LADSPA_Data from_level;
	LADSPA_Data level;
	ADENVState state;
	unsigned long samples;
} Dahdsr;

const LADSPA_Descriptor *
ladspa_descriptor(unsigned long index)
{
	if (index < ADENV_VARIANT_COUNT)
		return dahdsr_descriptors[index];

	return 0;
}

void
cleanupDahdsr(LADSPA_Handle instance)
{
	free(instance);
}

void
connectPortDahdsr(LADSPA_Handle instance,
				  unsigned long port, LADSPA_Data * data)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	switch (port) {
	case ADENV_GATE:
		plugin->gate = data;
		break;
	case ADENV_TRIGGER:
		plugin->trigger = data;
		break;
	case ADENV_ATTACK:
		plugin->attack = data;
		break;
	case ADENV_DECAY:
		plugin->decay = data;
		break;
	case ADENV_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateDahdsr(const LADSPA_Descriptor * descriptor,
				  unsigned long sample_rate)
{
	Dahdsr *plugin = (Dahdsr *) malloc(sizeof(Dahdsr));

	plugin->srate = (LADSPA_Data) sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LADSPA_Handle) plugin;
}

void
activateDahdsr(LADSPA_Handle instance)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	plugin->last_gate = 0.0f;
	plugin->last_trigger = 0.0f;
	plugin->from_level = 0.0f;
	plugin->level = 0.0f;
	plugin->state = IDLE;
	plugin->samples = 0;
}

void
runDahdsr_Control(LADSPA_Handle instance, unsigned long sample_count)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	/* Gate */
	LADSPA_Data *gate = plugin->gate;

	/* Trigger */
	LADSPA_Data *trigger = plugin->trigger;

	/* Attack Time (s) */
	LADSPA_Data attack = *(plugin->attack);

	/* Decay Time (s) */
	LADSPA_Data decay = *(plugin->decay);

	/* Envelope Out */
	LADSPA_Data *output = plugin->output;

	/* Instance Data */
	LADSPA_Data srate = plugin->srate;
	LADSPA_Data inv_srate = plugin->inv_srate;
	LADSPA_Data last_gate = plugin->last_gate;
	LADSPA_Data last_trigger = plugin->last_trigger;
	LADSPA_Data from_level = plugin->from_level;
	LADSPA_Data level = plugin->level;
	ADENVState state = plugin->state;
	unsigned long samples = plugin->samples;

	LADSPA_Data gat, trg, att, dec;
	LADSPA_Data elapsed;
	unsigned long s;

	/* Convert times into rates */
	att = attack > 0.0f ? inv_srate / attack : srate;
	dec = decay > 0.0f ? inv_srate / decay : srate;
	/* cuse's formula ...
	 * ReleaseCoeff = (ln(EndLevel) - ln(StartLevel)) / (EnvelopeDuration * SampleRate)
	 *
	 *  while (currentSample < endSample) Level += Level * ReleaseCoeff;
	 */

	LADSPA_Data ReleaseCoeff = log(0.001) / (decay * srate);

	for (s = 0; s < sample_count; s++) {
		gat = gate[s];
		trg = trigger[s];

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f)) ||
			(gat > 0.0f && !(last_gate > 0.0f))) {
			//fprintf(stderr, "triggered in control \n");
			if (att <= srate) {
				state = ATTACK;
			}
			samples = 0;
		}

		if (samples == 0)
			from_level = level;

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			level = 0;
			break;
		case ATTACK:
			samples++;
			elapsed = (LADSPA_Data) samples *att;

			if (elapsed > 1.0f) {
				state = DECAY;
				level = 1.0f;
				samples = 0;
			} else {
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case DECAY:
			samples++;
			elapsed = (LADSPA_Data) samples *dec;

			if (elapsed > 1.0f) {
				state = IDLE;
				level = 0.0f;
				samples = 0;
			} else {
				//fprintf(stderr, "decay, dec %f elapsed %f from level %f level %f\n", dec, elapsed, from_level, level);
				level += level * ReleaseCoeff;

			}
			break;
		default:
			/* Should never happen */
			fprintf(stderr, "bugger!!!");
			level = 0.0f;
		}

		output[s] = level;
		last_gate = gat;
		last_trigger = trg;
	}

	plugin->last_gate = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->from_level = from_level;
	plugin->level = level;
	plugin->state = state;
	plugin->samples = samples;
}

void
_init(void)
{
	static const unsigned long ids[] = { ADENV_BASE_ID };
	static const char *labels[] = { "adenv" };
	static const char *names[] = { G_NOP("Percussive AD Envelope") };
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;
	LADSPA_Descriptor *descriptor;

	LADSPA_PortDescriptor gate_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor trigger_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor attack_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor decay_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor output_port_descriptors[] =
		{ LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO };

	void (*run_functions[]) (LADSPA_Handle, unsigned long) = {
	runDahdsr_Control};

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	dahdsr_descriptors =
		(LADSPA_Descriptor **) calloc(ADENV_VARIANT_COUNT,
									  sizeof(LADSPA_Descriptor));

	if (dahdsr_descriptors) {
		int i = 0;

		dahdsr_descriptors[i] =
			(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
		descriptor = dahdsr_descriptors[i];
		if (descriptor) {
			descriptor->UniqueID = ids[i];
			descriptor->Label = labels[i];
			descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
			descriptor->Name = G_(names[i]);
			descriptor->Maker =
				"Loki Davison <ltdav1[at]student.monash.edu.au>";
			descriptor->Copyright = "GPL";

			descriptor->PortCount = 5;

			port_descriptors =
				(LADSPA_PortDescriptor *) calloc(5,
												 sizeof
												 (LADSPA_PortDescriptor));
			descriptor->PortDescriptors =
				(const LADSPA_PortDescriptor *)port_descriptors;

			port_range_hints =
				(LADSPA_PortRangeHint *) calloc(5,
												sizeof(LADSPA_PortRangeHint));
			descriptor->PortRangeHints =
				(const LADSPA_PortRangeHint *)port_range_hints;

			port_names = (char **)calloc(5, sizeof(char *));
			descriptor->PortNames = (const char **)port_names;

			/* Parameters for Gate */
			port_descriptors[ADENV_GATE] = gate_port_descriptors[i];
			port_names[ADENV_GATE] = G_("Gate");
			port_range_hints[ADENV_GATE].HintDescriptor =
				LADSPA_HINT_TOGGLED;

			/* Parameters for Trigger */
			port_descriptors[ADENV_TRIGGER] = trigger_port_descriptors[i];
			port_names[ADENV_TRIGGER] = G_("Trigger");
			port_range_hints[ADENV_TRIGGER].HintDescriptor =
				LADSPA_HINT_TOGGLED;

			/* Parameters for Attack Time (s) */
			port_descriptors[ADENV_ATTACK] = attack_port_descriptors[i];
			port_names[ADENV_ATTACK] = G_("Attack Time (s)");
			port_range_hints[ADENV_ATTACK].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[ADENV_ATTACK].LowerBound = 0.0f;

			/* Parameters for Decay Time (s) */
			port_descriptors[ADENV_DECAY] = decay_port_descriptors[i];
			port_names[ADENV_DECAY] = G_("Decay Time (s)");
			port_range_hints[ADENV_DECAY].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[ADENV_DECAY].LowerBound = 0.0f;

			/* Parameters for Envelope Out */
			port_descriptors[ADENV_OUTPUT] = output_port_descriptors[i];
			port_names[ADENV_OUTPUT] = G_("Envelope Out");
			port_range_hints[ADENV_OUTPUT].HintDescriptor = 0;

			descriptor->activate = activateDahdsr;
			descriptor->cleanup = cleanupDahdsr;
			descriptor->connect_port = connectPortDahdsr;
			descriptor->deactivate = NULL;
			descriptor->instantiate = instantiateDahdsr;
			descriptor->run = run_functions[i];
			descriptor->run_adding = NULL;
			descriptor->set_run_adding_gain = NULL;
		}
	}

}

void
_fini(void)
{
	LADSPA_Descriptor *descriptor;

	if (dahdsr_descriptors) {
		descriptor = dahdsr_descriptors[0];
		if (descriptor) {
			free((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
			free((char **)descriptor->PortNames);
			free((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
			free(descriptor);
		}
		free(dahdsr_descriptors);
	}
}
