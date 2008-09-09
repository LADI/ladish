#ifndef __mdaShepard_H
#define __mdaShepard_H

#include "audioeffectx.h"

class mdaShepard : public AudioEffectX
{
public:
	mdaShepard(audioMasterCallback audioMaster);
	~mdaShepard();

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
	float fParam0;
  float fParam1;
  float fParam2;
  float level, pos, rate, drate, out, filt;

  float *buf1, *buf2;
	long max, mode;

  char programName[32];
};

#endif
