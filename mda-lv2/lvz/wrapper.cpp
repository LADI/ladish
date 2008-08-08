/* LVZ - A C++ interface for writing LV2 plugins.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 *  
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PLUGIN_URI
#error "This file requires PLUGIN_URI to be defined"
#endif

#ifndef PLUGIN_CLASS
#error "This file requires PLUGIN_CLASS to be defined"
#endif

#include <stdlib.h>
#include "audioeffectx.h"
#include "lv2.h"
#include PLUGIN_HEADER

extern "C" {

/* Plugin */

typedef struct {
	PLUGIN_CLASS* effect;
	float**       inputs;
	float**       outputs;
} MDAPlugin;


static void mda_cleanup(LV2_Handle instance)
{
	free(instance);
}


static void mda_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;
	plugin->effect->setParameter(port, *(float*)data);
}


static LV2_Handle
mda_instantiate(const LV2_Descriptor*    descriptor,
                double                   rate,
                const char*              bundle_path,
                const LV2_Feature*const* features)
{
	PLUGIN_CLASS* effect = new PLUGIN_CLASS(NULL);
	effect->setSampleRate(rate);
	
	MDAPlugin* plugin = (MDAPlugin*)malloc(sizeof(MDAPlugin));
	plugin->effect = effect;
	plugin->inputs = (float**)malloc(sizeof(float*) * effect->getNumInputs());
	plugin->outputs = (float**)malloc(sizeof(float*) * effect->getNumOutputs());

	return (LV2_Handle)plugin;
}


static void
mda_run(LV2_Handle instance, uint32_t sample_count)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;
	plugin->effect->process(plugin->inputs, plugin->outputs, sample_count);
}

/* Library */

static LV2_Descriptor *mda_descriptor = NULL;

static void
init_descriptor()
{
	mda_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	mda_descriptor->URI = PLUGIN_URI;
	mda_descriptor->activate = NULL;
	mda_descriptor->cleanup = mda_cleanup;
	mda_descriptor->connect_port = mda_connect_port;
	mda_descriptor->deactivate = NULL;
	mda_descriptor->instantiate = mda_instantiate;
	mda_descriptor->run = mda_run;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!mda_descriptor)
		init_descriptor();

	switch (index) {
	case 0:
		return mda_descriptor;
	default:
		return NULL;
	}
}

} // extern "C"
