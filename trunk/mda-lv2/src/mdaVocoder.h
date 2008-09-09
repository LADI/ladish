//See associated .cpp file for copyright and other info


#define NPARAMS  8  ///number of parameters
#define NPROGS   5  ///number of programs
#define NBANDS  16  ///max vocoder bands

#ifndef __mdaVocoder_H
#define __mdaVocoder_H

#include "audioeffectx.h"


class mdaVocoderProgram
{
public:
  mdaVocoderProgram();
  ~mdaVocoderProgram() {}

private:
  friend class mdaVocoder;
  float param[NPARAMS];
  char name[32];
};


class mdaVocoder : public AudioEffectX
{
public:
  mdaVocoder(audioMasterCallback audioMaster);
  ~mdaVocoder();

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
  mdaVocoderProgram *programs;

  ///global internal variables
  long  swap;       //input channel swap
  float gain;       //output level
  float thru, high; //hf thru              
  float kout; //downsampled output
  long  kval; //downsample counter
  long  nbnd; //number of bands
  
  //filter coeffs and buffers - seems it's faster to leave this global than make local copy 
  float f[NBANDS][13]; //[0-8][0 1 2 | 0 1 2 3 | 0 1 2 3 | val rate]
};                     //  #   reson | carrier |modulator| envelope

#endif
