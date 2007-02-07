/*
    dahdsr_fexp.c - A LADSPA plugin to generate DAHDSR envelopes
                  exponential attack, decay and release version. 
    Copyright (C) 2005 Loki Davison, based on DAHDSR by Mike Rawes

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define _XOPEN_SOURCE 500		/* strdup */
#include <stdlib.h>
#include <ladspa.h>
#include <math.h>

#ifdef ENABLE_NLS
#  include <locale.h>
#  define G_(s) gettext(s)
#else
#  define G_(s) (s)
#endif
#define G_NOP(s) s

#define DAHDSR_VARIANT_COUNT             1

#define DAHDSR_GATE                      0
#define DAHDSR_TRIGGER                   1
#define DAHDSR_DELAY                     2
#define DAHDSR_ATTACK                    3
#define DAHDSR_HOLD                      4
#define DAHDSR_DECAY                     5
#define DAHDSR_SUSTAIN                   6
#define DAHDSR_RELEASE                   7
#define DAHDSR_OUTPUT                    8

LADSPA_Descriptor **dahdsr_descriptors = 0;

typedef enum {
	IDLE,
	DELAY,
	ATTACK,
	HOLD,
	DECAY,
	SUSTAIN,
	RELEASE
} DAHDSRState;

typedef struct {
	LADSPA_Data *gate;
	LADSPA_Data *trigger;
	LADSPA_Data *delay;
	LADSPA_Data *attack;
	LADSPA_Data *hold;
	LADSPA_Data *decay;
	LADSPA_Data *sustain;
	LADSPA_Data *release;
	LADSPA_Data *output;
	LADSPA_Data srate;
	LADSPA_Data inv_srate;
	LADSPA_Data last_gate;
	LADSPA_Data last_trigger;
	LADSPA_Data from_level;
	LADSPA_Data level;
	DAHDSRState state;
	unsigned long samples;
} Dahdsr;

const LADSPA_Descriptor *
ladspa_descriptor(unsigned long index)
{
	if (index < DAHDSR_VARIANT_COUNT)
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
	case DAHDSR_GATE:
		plugin->gate = data;
		break;
	case DAHDSR_TRIGGER:
		plugin->trigger = data;
		break;
	case DAHDSR_DELAY:
		plugin->delay = data;
		break;
	case DAHDSR_ATTACK:
		plugin->attack = data;
		break;
	case DAHDSR_HOLD:
		plugin->hold = data;
		break;
	case DAHDSR_DECAY:
		plugin->decay = data;
		break;
	case DAHDSR_SUSTAIN:
		plugin->sustain = data;
		break;
	case DAHDSR_RELEASE:
		plugin->release = data;
		break;
	case DAHDSR_OUTPUT:
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

	/* Delay Time (s) */
	LADSPA_Data delay = *(plugin->delay);

	/* Attack Time (s) */
	LADSPA_Data attack = *(plugin->attack);

	/* Hold Time (s) */
	LADSPA_Data hold = *(plugin->hold);

	/* Decay Time (s) */
	LADSPA_Data decay = *(plugin->decay);

	/* Sustain Level */
	LADSPA_Data sustain = *(plugin->sustain);

	/* Release Time (s) */
	LADSPA_Data release = *(plugin->release);

	/* Envelope Out */
	LADSPA_Data *output = plugin->output;

	/* Instance Data */
	LADSPA_Data srate = plugin->srate;
	LADSPA_Data inv_srate = plugin->inv_srate;
	LADSPA_Data last_gate = plugin->last_gate;
	LADSPA_Data last_trigger = plugin->last_trigger;
	LADSPA_Data from_level = plugin->from_level;
	LADSPA_Data level = plugin->level;
	DAHDSRState state = plugin->state;
	unsigned long samples = plugin->samples;

	LADSPA_Data gat, trg, del, att, hld, dec, sus, rel;
	LADSPA_Data elapsed;
	unsigned long s;

	/* Convert times into rates */
	del = delay > 0.0f ? inv_srate / delay : srate;
	att = attack > 0.0f ? inv_srate / attack : srate;
	hld = hold > 0.0f ? inv_srate / hold : srate;
	dec = decay > 0.0f ? inv_srate / decay : srate;
	rel = release > 0.0f ? inv_srate / release : srate;
	sus = sustain;

	if (sus == 0.0f) {
		sus = 0.001f;
	}
	if (sus > 1.0f) {
		sus = 1.0f;
	}

	LADSPA_Data ReleaseCoeff_att = (0 - log(0.001)) / (attack * srate);
	LADSPA_Data ReleaseCoeff_dec = (log(sus)) / (decay * srate);
	LADSPA_Data ReleaseCoeff_rel =
		(log(0.001) - log(sus)) / (release * srate);

	for (s = 0; s < sample_count; s++) {
		gat = gate[s];
		trg = trigger[s];

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f)) ||
			(gat > 0.0f && !(last_gate > 0.0f))) {
			if (del < srate) {
				state = DELAY;
			} else if (att < srate) {
				state = ATTACK;
			} else {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE : IDLE)));
				level = 1.0f;
			}
			samples = 0;
		}

		/* Release if gate was open and now closed */
		if (state != IDLE && state != RELEASE &&
			last_gate > 0.0f && !(gat > 0.0f)) {
			state = rel < srate ? RELEASE : IDLE;
			samples = 0;
		}

		if (samples == 0)
			from_level = level;

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			level = 0;
			break;
		case DELAY:
			samples++;
			elapsed = (LADSPA_Data) samples *del;

			if (elapsed > 1.0f) {
				state = att < srate ? ATTACK
					: (hld < srate ? HOLD
					   : (dec < srate ? DECAY
						  : (gat > 0.0f ? SUSTAIN
							 : (rel < srate ? RELEASE : IDLE))));
				samples = 0;
			}
			break;
		case ATTACK:
			samples++;
			elapsed = (LADSPA_Data) samples *att;

			if (elapsed > 1.0f) {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE : IDLE)));
				level = 1.0f;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_att;
			}
			break;
		case HOLD:
			samples++;
			elapsed = (LADSPA_Data) samples *hld;

			if (elapsed > 1.0f) {
				state = dec < srate ? DECAY
					: (gat > 0.0f ? SUSTAIN : (rel < srate ? RELEASE : IDLE));
				samples = 0;
			}
			break;
		case DECAY:
			samples++;
			elapsed = (LADSPA_Data) samples *dec;

			if (elapsed > 1.0f) {
				state = gat > 0.0f ? SUSTAIN : (rel < srate ? RELEASE : IDLE);
				level = sus;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_dec;
			}
			break;
		case SUSTAIN:
			level = sus;
			break;
		case RELEASE:
			samples++;
			elapsed = (LADSPA_Data) samples *rel;

			if (elapsed > 1.0f) {
				state = IDLE;
				level = 0.0f;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_rel;
			}
			break;
		default:
			/* Should never happen */
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
	static const unsigned long ids[] = { 2664 };
	static const char *labels[] = { "dahdsr_fexp" };
	static const char *names[] = { G_NOP("DAHDSR Envelope full exp, adr") };
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;
	LADSPA_Descriptor *descriptor;

	LADSPA_PortDescriptor gate_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor trigger_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO };
	LADSPA_PortDescriptor delay_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor attack_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor hold_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor decay_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor sustain_port_descriptors[] =
		{ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL };
	LADSPA_PortDescriptor release_port_descriptors[] =
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
		(LADSPA_Descriptor **) calloc(DAHDSR_VARIANT_COUNT,
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

			descriptor->PortCount = 9;

			port_descriptors =
				(LADSPA_PortDescriptor *) calloc(9,
												 sizeof
												 (LADSPA_PortDescriptor));
			descriptor->PortDescriptors =
				(const LADSPA_PortDescriptor *)port_descriptors;

			port_range_hints =
				(LADSPA_PortRangeHint *) calloc(9,
												sizeof(LADSPA_PortRangeHint));
			descriptor->PortRangeHints =
				(const LADSPA_PortRangeHint *)port_range_hints;

			port_names = (char **)calloc(9, sizeof(char *));
			descriptor->PortNames = (const char **)port_names;

			/* Parameters for Gate */
			port_descriptors[DAHDSR_GATE] = gate_port_descriptors[i];
			port_names[DAHDSR_GATE] = G_("Gate");
			port_range_hints[DAHDSR_GATE].HintDescriptor =
				LADSPA_HINT_TOGGLED;

			/* Parameters for Trigger */
			port_descriptors[DAHDSR_TRIGGER] = trigger_port_descriptors[i];
			port_names[DAHDSR_TRIGGER] = G_("Trigger");
			port_range_hints[DAHDSR_TRIGGER].HintDescriptor =
				LADSPA_HINT_TOGGLED;

			/* Parameters for Delay Time (s) */
			port_descriptors[DAHDSR_DELAY] = delay_port_descriptors[i];
			port_names[DAHDSR_DELAY] = G_("Delay Time (s)");
			port_range_hints[DAHDSR_DELAY].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[DAHDSR_DELAY].LowerBound = 0.0f;

			/* Parameters for Attack Time (s) */
			port_descriptors[DAHDSR_ATTACK] = attack_port_descriptors[i];
			port_names[DAHDSR_ATTACK] = G_("Attack Time (s)");
			port_range_hints[DAHDSR_ATTACK].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[DAHDSR_ATTACK].LowerBound = 0.0f;

			/* Parameters for Hold Time (s) */
			port_descriptors[DAHDSR_HOLD] = hold_port_descriptors[i];
			port_names[DAHDSR_HOLD] = G_("Hold Time (s)");
			port_range_hints[DAHDSR_HOLD].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[DAHDSR_HOLD].LowerBound = 0.0f;

			/* Parameters for Decay Time (s) */
			port_descriptors[DAHDSR_DECAY] = decay_port_descriptors[i];
			port_names[DAHDSR_DECAY] = G_("Decay Time (s)");
			port_range_hints[DAHDSR_DECAY].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[DAHDSR_DECAY].LowerBound = 0.0f;

			/* Parameters for Sustain Level */
			port_descriptors[DAHDSR_SUSTAIN] = sustain_port_descriptors[i];
			port_names[DAHDSR_SUSTAIN] = G_("Sustain Level");
			port_range_hints[DAHDSR_SUSTAIN].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				LADSPA_HINT_DEFAULT_MAXIMUM;
			port_range_hints[DAHDSR_SUSTAIN].LowerBound = 0.0f;
			port_range_hints[DAHDSR_SUSTAIN].UpperBound = 1.0f;

			/* Parameters for Release Time (s) */
			port_descriptors[DAHDSR_RELEASE] = release_port_descriptors[i];
			port_names[DAHDSR_RELEASE] = G_("Release Time (s)");
			port_range_hints[DAHDSR_RELEASE].HintDescriptor =
				LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
			port_range_hints[DAHDSR_RELEASE].LowerBound = 0.0f;

			/* Parameters for Envelope Out */
			port_descriptors[DAHDSR_OUTPUT] = output_port_descriptors[i];
			port_names[DAHDSR_OUTPUT] = G_("Envelope Out");
			port_range_hints[DAHDSR_OUTPUT].HintDescriptor = 0;

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
	int i;

	if (dahdsr_descriptors) {
		for (i = 0; i < DAHDSR_VARIANT_COUNT; i++) {
			descriptor = dahdsr_descriptors[i];
			if (descriptor) {
				free((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free((char **)descriptor->PortNames);
				free((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free(descriptor);
			}
		}
		free(dahdsr_descriptors);
	}
}
