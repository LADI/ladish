#ifndef __mdaOverdrive_H
#define __mdaOverdrive_H

#include "audioeffectx.h"

class mdaOverdrive : public AudioEffectX
{
public:
	mdaOverdrive(audioMasterCallback audioMaster);
	~mdaOverdrive();

	virtual void process(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(LvzInt32 index, float value);
	virtual float getParameter(LvzInt32 index);
	virtual void getParameterLabel(LvzInt32 index, char *label);
	virtual void getParameterDisplay(LvzInt32 index, char *text);
	virtual void getParameterName(LvzInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual LvzInt32 getVendorVersion() { return 1000; }
  
protected:
	float fParam1;
  float fParam2;
  float fParam3;

  float filt1, filt2; //filter buffers
  float filt;         //filter coeff.
  float gain;         //output gain

	char programName[32];
};

#endif
