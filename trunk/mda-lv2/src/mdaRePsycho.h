#ifndef __mdaRePsycho_H
#define __mdaRePsycho_H

#include "audioeffectx.h"

class mdaRePsycho : public AudioEffectX
{
public:
	mdaRePsycho(audioMasterCallback audioMaster);
	~mdaRePsycho();

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
  float fParam7;
  float thr, env, gai, tun, wet, dry, fil, buf, buf2;
  long tim, dtim;

  float *buffer, *buffer2;
	long size;

	char programName[32];
};

#endif
