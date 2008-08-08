//See associated .cpp file for copyright and other info


#define NPARAMS  7  ///number of parameters
#define NPROGS   3  ///number of programs

#ifndef __mdaSplitter_H
#define __mdaSplitter_H

#include "audioeffectx.h"

class mdaSplitterProgram
{
public:
  mdaSplitterProgram();
  ~mdaSplitterProgram() {}

private:
  friend class mdaSplitter;
  float param[NPARAMS];
  char name[32];
};


class mdaSplitter : public AudioEffectX
{
public:
  mdaSplitter(audioMasterCallback audioMaster);
  ~mdaSplitter();

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
  mdaSplitterProgram *programs;

  ///global internal variables
  float freq, fdisp, buf0, buf1, buf2, buf3;  //filter
  float level, ldisp, env, att, rel;          //level switch
  float ff, ll, pp, i2l, i2r, o2l, o2r;       //routing (freq, level, phase, output)
  long  mode;

};

#endif
