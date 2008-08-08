//See associated .cpp file for copyright and other info


#define NPARAMS  5  ///number of parameters
#define NPROGS   1  ///number of programs

#ifndef __mdaVocInput_H
#define __mdaVocInput_H

#include "audioeffectx.h"


class mdaVocInputProgram
{
public:
  mdaVocInputProgram();
  ~mdaVocInputProgram() {}

private:
  friend class mdaVocInput;
  float param[NPARAMS];
  char name[32];
};


class mdaVocInput : public AudioEffectX
{
public:
  mdaVocInput(audioMasterCallback audioMaster);
  ~mdaVocInput();

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
  virtual void  midi2string(long n, char *text);
  virtual void  suspend();
  virtual void  resume();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual LvzInt32 getVendorVersion() { return 1000; }

protected:
  float param[NPARAMS];
  char programName[32];
  mdaVocInputProgram *programs;

  ///global internal variables
  long  track;        //track input pitch
  float pstep;        //output sawtooth inc per sample
  float pmult;        //tuning multiplier
  float sawbuf;   
  float noise;        //breath noise level
  float lenv, henv;   //LF and overall envelope
  float lbuf0, lbuf1; //LF filter buffers
  float lbuf2;        //previous LF sample
  float lbuf3;        //period measurement
  float lfreq;        //LF filter coeff
  float vuv;          //voiced/unvoiced threshold
  float maxp, minp;   //preferred period range
  double root;        //tuning reference (MIDI note 0 in Hz)
};

#endif
