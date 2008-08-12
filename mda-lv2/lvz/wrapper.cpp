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

#ifndef PLUGIN_CLASS
#error "This file requires PLUGIN_CLASS to be defined"
#endif
#ifndef URI_PREFIX
#error "This file requires URI_PREFIX to be defined"
#endif
#ifndef PLUGIN_URI_SUFFIX
#error "This file requires PLUGIN_URI_SUFFIX to be defined"
#endif

#include <stdlib.h>
#include "audioeffectx.h"
#include "lv2.h"
#include PLUGIN_HEADER

extern "C" {

/* Plugin */

typedef struct {
	PLUGIN_CLASS* effect;
	float*        controls;
	float**       control_buffers;
	float**       inputs;
	float**       outputs;
} MDAPlugin;


static void
mda_cleanup(LV2_Handle instance)
{
	free(instance);
}


static void
mda_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;

	uint32_t num_params = plugin->effect->getNumParameters();
	uint32_t num_inputs = plugin->effect->getNumInputs();
	
	if (port < num_params) {
		plugin->control_buffers[port] = (float*)data;
		if (data != NULL)
			plugin->controls[port] = *(float*)data;
	} else if (port < num_params + num_inputs) {
		plugin->inputs[port - num_params] = (float*)data;
	} else {
		plugin->outputs[port - num_params - num_inputs] = (float*)data;
	}
}


static int
master_callback(int, int ver, int, int, int, int)
{
	return 0;
}


static LV2_Handle
mda_instantiate(const LV2_Descriptor*    descriptor,
                double                   rate,
                const char*              bundle_path,
                const LV2_Feature*const* features)
{
	PLUGIN_CLASS* effect = new PLUGIN_CLASS(master_callback);
	effect->setURI(URI_PREFIX PLUGIN_URI_SUFFIX);
	effect->setSampleRate(rate);
	
	uint32_t num_params = effect->getNumParameters();
	uint32_t num_inputs = effect->getNumInputs();
	uint32_t num_outputs = effect->getNumOutputs();
	
	MDAPlugin* plugin = (MDAPlugin*)malloc(sizeof(MDAPlugin));
	plugin->effect = effect;
	
	if (num_params > 0) {
		plugin->controls = (float*)malloc(sizeof(float) * num_params);
		plugin->control_buffers = (float**)malloc(sizeof(float*) * num_params);
		for (uint32_t i = 0; i < num_params; ++i) {
			plugin->controls[i] = effect->getParameter(i);
			plugin->control_buffers[i] = NULL;
		}
	} else {
		plugin->controls = NULL;
		plugin->control_buffers = NULL;
	}

	if (num_inputs > 0) {
		plugin->inputs = (float**)malloc(sizeof(float*) * num_inputs);
		for (uint32_t i = 0; i < num_inputs; ++i)
			plugin->inputs[i] = NULL;
	} else {
		plugin->inputs = NULL;
	}
	
	if (num_outputs > 0) {
		plugin->outputs = (float**)malloc(sizeof(float*) * num_outputs);
		for (uint32_t i = 0; i < num_outputs; ++i)
			plugin->outputs[i] = NULL;
	} else {
		plugin->outputs = NULL;
	}
	
	return (LV2_Handle)plugin;
}


static void
mda_run(LV2_Handle instance, uint32_t sample_count)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;
	
	for (int32_t i = 0; i < plugin->effect->getNumParameters(); ++i) {
		float val = plugin->control_buffers[i][0];
		if (val != plugin->controls[i]) {
			plugin->effect->setParameter(i, val);
			plugin->controls[i] = val;
		}
	}
	
	plugin->effect->processReplacing(plugin->inputs, plugin->outputs, sample_count);
}


static const AudioEffectX*
mda_get_audioeffectx(LV2_Handle instance)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;
	return plugin->effect;
}


static const void*
mda_extension_data(const char* uri)
{
	if (!strcmp(uri, "http://drobilla.net/ns/dev/vstgui")) {
		// FIXME: shouldn't return function pointers directly
		return (const void*)mda_get_audioeffectx;
	} else {
		return NULL;
	}
}
	

static void
mda_deactivate(LV2_Handle instance)
{
	MDAPlugin* plugin = (MDAPlugin*)instance;
	plugin->effect->suspend();
}


/* Library */

static LV2_Descriptor *mda_descriptor = NULL;

static void
init_descriptor()
{
	mda_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	mda_descriptor->URI = URI_PREFIX PLUGIN_URI_SUFFIX;
	mda_descriptor->instantiate = mda_instantiate;
	mda_descriptor->connect_port = mda_connect_port;
	mda_descriptor->activate = NULL;
	mda_descriptor->run = mda_run;
	mda_descriptor->deactivate = mda_deactivate;
	mda_descriptor->cleanup = mda_cleanup;
	mda_descriptor->extension_data = mda_extension_data;
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


LV2_SYMBOL_EXPORT
AudioEffectX*
lvz_new_audioeffectx()
{
	PLUGIN_CLASS* effect = new PLUGIN_CLASS(master_callback);
	effect->setURI(URI_PREFIX PLUGIN_URI_SUFFIX);
	return effect;
}


} // extern "C"

