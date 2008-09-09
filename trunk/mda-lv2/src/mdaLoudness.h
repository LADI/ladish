//See associated .cpp file for copyright and other info


#define NPARAMS  3  ///number of parameters
#define NPROGS   8  ///number of programs

#ifndef __mdaLoudness_H
#define __mdaLoudness_H

#include "audioeffectx.h"

class mdaLoudnessProgram
{
public:
  mdaLoudnessProgram();
  ~mdaLoudnessProgram() {}

private:
  friend class mdaLoudness;
  float param[NPARAMS];
  char name[32];
};


class mdaLoudness : public AudioEffectX
{
public:
  mdaLoudness(audioMasterCallback audioMaster);
  ~mdaLoudness();

  virtual void  process(float **inputs, float **outputs, LvzInt32 sampleFrames);
  virtual void  processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
  virtual void  setProgram(LvzInt32 program);
  virtual void  setProgramName(char *name);
  virtual void  getProgramName(char *name);
  virtual void  setParameter(LvzInt32 index, float value);
  virtual float getParameter(LvzInt32 index);
  virtual void  getParameterLabel(LvzInt32 index, char *label);
  virtual void  getParameterDisplay(LvzInt32 index, char *text);
  virtual void  getParameterName(LvzInt32 index, char *text);
  virtual void  suspend();
  virtual void  resume();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual LvzInt32 getVendorVersion() { return 1000; }

protected:
  float param[NPARAMS];
  char programName[32];
  mdaLoudnessProgram *programs;

  ///global internal variables
  float Z0, Z1, Z2, Z3, A0, A1, A2, gain;
  float igain, ogain;
  long  mode;
};

#endif
