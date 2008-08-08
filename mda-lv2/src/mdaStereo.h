#ifndef __mdaStereo_H
#define __mdaStereo_H

#include "audioeffectx.h"

class mdaStereo : public AudioEffectX
{
public:
	mdaStereo(audioMasterCallback audioMaster);
	~mdaStereo();

	virtual void process(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(LvzInt32 index, float value);
	virtual float getParameter(LvzInt32 index);
	virtual void getParameterLabel(LvzInt32 index, char *label);
	virtual void getParameterDisplay(LvzInt32 index, char *text);
	virtual void getParameterName(LvzInt32 index, char *text);
	virtual void suspend();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual LvzInt32 getVendorVersion() { return 1000; }

protected:
	float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;

  float fli, fld, fri, frd, fdel, phi, dphi, mod;
  
  float *buffer;
	long size, bufpos;

	char programName[32];
};

#endif
