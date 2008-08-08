#ifndef __mdaTracker_H
#define __mdaTracker_H

#include "audioeffectx.h"

class mdaTracker : public AudioEffectX
{
public:
	mdaTracker(audioMasterCallback audioMaster);
	~mdaTracker();

	virtual void process(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(LvzInt32 index, float value);
	virtual float getParameter(LvzInt32 index);
	virtual void getParameterLabel(LvzInt32 index, char *label);
	virtual void getParameterDisplay(LvzInt32 index, char *text);
	virtual void getParameterName(LvzInt32 index, char *text);
  virtual float filterFreq(float hz);
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
  float fParam8;
  float fi, fo, thr, phi, dphi, ddphi, trans;
  float buf1, buf2, dn, bold, wet, dry;
  float dyn, env, rel, saw, dsaw;
  float res1, res2, buf3, buf4;
  long  max, min, num, sig, mode;
    
	char programName[32];
};

#endif
