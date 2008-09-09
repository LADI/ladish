//See associated .cpp file for copyright and other info

#ifndef __mdaLooplex__
#define __mdaLooplex__

#include <string.h>

#include "audioeffectx.h"

#define NPARAMS  7       //number of parameters
#define NPROGS   1       //number of programs
#define NOUTS    2       //number of outputs
#define SILENCE  0.00001f


class mdaLooplexProgram
{
  friend class mdaLooplex;
public:
	mdaLooplexProgram();
	~mdaLooplexProgram() {}

private:
  float param[NPARAMS];
  char  name[24];
};


class mdaLooplex : public AudioEffectX
{
public:
	mdaLooplex(audioMasterCallback audioMaster);
	~mdaLooplex();

	virtual void process(float **inputs, float **outputs, LvzInt32 sampleframes);
	virtual void processReplacing(float **inputs, float **outputs, LvzInt32 sampleframes);
	virtual LvzInt32 processEvents(LvzEvents* events);

	virtual void setProgram(LvzInt32 program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(LvzInt32 index, float value);
	virtual float getParameter(LvzInt32 index);
	virtual void getParameterLabel(LvzInt32 index, char *label);
	virtual void getParameterDisplay(LvzInt32 index, char *text);
	virtual void getParameterName(LvzInt32 index, char *text);
	virtual void setSampleRate(float sampleRate);
	virtual void setBlockSize(LvzInt32 blockSize);
    virtual void resume();

	virtual bool getProgramNameIndexed (LvzInt32 category, LvzInt32 index, char* text);
	virtual bool copyProgram (LvzInt32 destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual LvzInt32 getVendorVersion () {return 1;}
	virtual LvzInt32 canDo (char* text);

	virtual LvzInt32 getNumMidiInputChannels ()  { return 1; }
	
  void idle();
  
private:
	void update();  //my parameter update

  float param[NPARAMS];
  mdaLooplexProgram* programs;
  float Fs;

  #define EVENTBUFFER 120
  #define EVENTS_DONE 99999999
  long notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

  ///global internal variables
  float in_mix, in_pan, out_mix, feedback, modwhl;

  short *buffer;
  long bufpos, buflen, bufmax, mode;

  long bypass, bypassed, busy, status, recreq;
  float oldParam0, oldParam1, oldParam2;

};

#endif
