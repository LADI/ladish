#ifndef __mdaBeatBox_H
#define __mdaBeatBox_H

#include "audioeffectx.h"

class mdaBeatBox : public AudioEffectX
{
public:
	mdaBeatBox(audioMasterCallback audioMaster);
	~mdaBeatBox();

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
  virtual void synth();

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
  float fParam9;
  float fParam10;
  float fParam11;
  float fParam12;
  float hthr, hfil, sthr, kthr, kfil1, kfil2, mix;
  float klev, hlev, slev;
  float  ww,  wwx,  sb1,  sb2,  sf1,  sf2, sf3;
  float kww, kwwx, ksb1, ksb2, ksf1, ksf2;
  float dyne, dyna, dynr, dynm;

  float *hbuf;
  float *kbuf;
  float *sbuf, *sbuf2;
	long hbuflen, hbufpos, hdel;
	long sbuflen, sbufpos, sdel, sfx;
  long kbuflen, kbufpos, kdel, ksfx;
  long rec, recx, recpos;

	char programName[32];
};

#endif
