#ifndef __mdaLeslie_H
#define __mdaLeslie_H

#include "audioeffectx.h"

class mdaLeslieProgram
{
public:
	mdaLeslieProgram();
	~mdaLeslieProgram() {}

private:	
	friend class mdaLeslie;
	float fParam1, fParam3, fParam4, fParam5, fParam6; 
  float fParam7, fParam9, fParam2, fParam8;
	char name[24];
};

class mdaLeslie : public AudioEffectX
{
public:
	mdaLeslie(audioMasterCallback audioMaster);
	~mdaLeslie();

	virtual void process(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
	virtual void setProgram(LvzInt32 program);
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
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float fParam9;
  float fParam2;
  float fParam8;

	mdaLeslieProgram *programs;

  float filo; //crossover filter coeff
  float fbuf1, fbuf2; //filter buffers
  float twopi; //speed, target, momentum, phase, width, ampmod, freqmod...
  float hspd, hset, hmom, hphi, hwid, hlev, hdep; 
  float lspd, lset, lmom, lphi, lwid, llev, gain;
  float *hbuf;  //HF delay buffer
	long size, hpos; //buffer length & pointer
  
  float chp, dchp, clp, dclp, shp, dshp, slp, dslp;
  
	char programName[32];
};

#endif
