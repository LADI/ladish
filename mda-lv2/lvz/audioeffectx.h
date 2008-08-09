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

#ifndef __lvz_audioeffectx_h
#define __lvz_audioeffectx_h

#include <stdint.h>
#include <string.h>

typedef int16_t LvzInt16;
typedef int32_t LvzInt32;
typedef void*   audioMasterCallback;
	 
class AEffGUIEditor;

enum LvzPinFlags {
	kLvzPinIsActive = 1<<0,
	kLvzPinIsStereo = 1<<1
};

struct LvzPinProperties {
	LvzPinProperties() : label(NULL), flags(0) {}
	char* label;
	int   flags;

};

enum LvzEventTypes {
	kLvzMidiType = 0
};

struct LvzEvent {
	int type;
};

struct LvzMidiEvent : public LvzEvent {
	char* midiData;
	long deltaFrames;
};

struct LvzEvents {
	long numEvents;
	LvzEvent** events;
};

#define DECLARE_LVZ_DEPRECATED(x) x

class AudioEffect {
};

class AudioEffectX : public AudioEffect {
public:
	AudioEffectX(audioMasterCallback audioMaster, int progs, int params)
		: uniqueID("NIL")
		, URI("NIL")
		, curProgram(0)
		, numPrograms(progs)
		, numParams(params)
		, numInputs(0)
		, numOutputs(0)
		, sampleRate(44100)
	{
	}
  
	const char*  getURI()           { return URI; }
	const char*  getUniqueID()      { return uniqueID; }
	float        getSampleRate()    { return sampleRate; }
	uint32_t     getNumInputs()     { return numInputs; }		  
	uint32_t     getNumOutputs()    { return numOutputs; }
	uint32_t     getNumParameters() { return numParams; }

	virtual void getParameterName(LvzInt32 index, char *label) = 0;
	virtual bool getProductString(char* text) = 0;

	void canMono() {}
	void canProcessReplacing() {}
	void isSynth() {}
	void process(float **inputs, float **outputs, uint32_t nframes) {}
	void setBlockSize(uint32_t blockSize) {}
	void setNumInputs(uint32_t num) { numInputs = num; }
	void setNumOutputs(uint32_t num) { numOutputs = num;}
	void setParameter(uint32_t index, float value) {}
	void setSampleRate(float rate) { sampleRate = rate; }
	void setUniqueID(const char* id) { uniqueID = id; }
	void setURI(const char* uri) { URI = uri; }
	void suspend() {}
	void wantEvents() {}

protected:
	const char* uniqueID;
	const char* URI;
	uint32_t curProgram;
	uint32_t numPrograms;
	uint32_t numParams;
	uint32_t numInputs;
	uint32_t numOutputs;
	float    sampleRate;

	AEffGUIEditor* editor;
};

#endif // __lvz_audioeffectx_h

