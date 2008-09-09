//See associated .cpp file for copyright and other info


#define NPARAMS  5  ///number of parameters
#define NPROGS   4  ///number of programs
#define BUFMAX   2048

#ifndef __mdaThruZero_H
#define __mdaThruZero_H

#include "audioeffectx.h"


class mdaThruZeroProgram
{
public:
  mdaThruZeroProgram();
  ~mdaThruZeroProgram() {}

private:
  friend class mdaThruZero;
  float param[NPARAMS];
  char name[32];
};


class mdaThruZero : public AudioEffectX
{
public:
  mdaThruZero(audioMasterCallback audioMaster);
  ~mdaThruZero();

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
  mdaThruZeroProgram *programs;

  ///global internal variables
  float rat, dep, wet, dry, fb, dem; //rate, depth, wet & dry mix, feedback, mindepth
  float phi, fb1, fb2, deps;         //lfo & feedback buffers, depth change smoothing 
  float *buffer, *buffer2;           //maybe use 2D buffer?
	long size, bufpos;
};

#endif
