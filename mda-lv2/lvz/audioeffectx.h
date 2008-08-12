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

#ifndef __LVZ_AUDIOEFFECTX_H
#define __LVZ_AUDIOEFFECTX_H

#include <stdint.h>
#include <string.h>

// Some plugins seem to use these names...
#ifndef VstInt32
#  define VstInt32 LvzInt32
#  define VstInt16 LvzInt16
#endif
#define VstEvents LvzEvents
#define VstMidiEvent LvzMidiEvent
#define VstPinProperty LvzPinProperty

typedef int16_t LvzInt16;
typedef int32_t LvzInt32;
typedef int (*audioMasterCallback)(int, int ver, int, int, int, int);
	 
class AEffEditor;

struct VstFileSelect {
	int reserved;
	char* returnPath;
	size_t sizeReturnPath;
	char** returnMultiplePaths;
	long nbReturnPath;
};

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
	LvzInt32 deltaFrames;
};

struct LvzEvents {
	LvzInt32 numEvents;
	LvzEvent** events;
};

#define DECLARE_LVZ_DEPRECATED(x) x

class AudioEffect {
public:
	AudioEffect() : editor(NULL) {}
	virtual ~AudioEffect() {}
	
	virtual void  setParameter(LvzInt32 index, float value) = 0;
	virtual float getParameter(LvzInt32 index)              = 0;

	void setEditor(AEffEditor* e) { editor = e; }
	virtual void masterIdle()                {}
protected:
	AEffEditor* editor;
};


class AudioEffectX : public AudioEffect {
public:
	AudioEffectX(audioMasterCallback audioMaster, LvzInt32 progs, LvzInt32 params)
		: URI("NIL")
		, uniqueID("NIL")
		, sampleRate(44100)
		, curProgram(0)
		, numInputs(0)
		, numOutputs(0)
		, numParams(params)
		, numPrograms(progs)
	{
	}
	
	virtual void process         (float **inputs, float **outputs, LvzInt32 nframes) = 0;
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 nframes) = 0;

	virtual const char*  getURI()           { return URI; }
	virtual const char*  getUniqueID()      { return uniqueID; }
	virtual float        getSampleRate()    { return sampleRate; }
	virtual LvzInt32     getNumInputs()     { return numInputs; }		  
	virtual LvzInt32     getNumOutputs()    { return numOutputs; }
	virtual LvzInt32     getNumParameters() { return numParams; }

	virtual void  getParameterName(LvzInt32 index, char *label) = 0;
	virtual bool  getProductString(char* text)                  = 0;

	virtual bool canHostDo(const char* act) { return false; }
	virtual void canMono()                  {}
	virtual void canProcessReplacing()      {}
	virtual void isSynth()                  {}
	virtual void wantEvents()               {}

	virtual void setBlockSize(LvzInt32 size) {}
	virtual void setNumInputs(LvzInt32 num)  { numInputs = num; }
	virtual void setNumOutputs(LvzInt32 num) { numOutputs = num; }
	virtual void setSampleRate(float rate)   { sampleRate = rate; }
	virtual void setProgram(LvzInt32 prog)   { curProgram = prog; }
	virtual void setURI(const char* uri)     { URI = uri; }
	virtual void setUniqueID(const char* id) { uniqueID = id; }
	virtual void suspend()                   {}
	virtual void beginEdit(VstInt32 index)   {}
	virtual void endEdit(VstInt32 index)     {}

	virtual bool openFileSelector (VstFileSelect* sel)  { return false; }
	virtual bool closeFileSelector (VstFileSelect* sel) { return false; }

protected:
	const char* URI;
	const char* uniqueID;
	float       sampleRate;
	LvzInt32    curProgram;
	LvzInt32    numInputs;
	LvzInt32    numOutputs;
	LvzInt32    numParams;
	LvzInt32    numPrograms;
};

#endif // __LVZ_AUDIOEFFECTX_H

