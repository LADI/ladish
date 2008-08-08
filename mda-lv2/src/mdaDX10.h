//See associated .cpp file for copyright and other info

#ifndef __mdaDX10__
#define __mdaDX10__

#include <string.h>

#include "audioeffectx.h"

#define NPARAMS 16       //number of parameters
#define NPROGS  32       //number of programs
#define NOUTS    2       //number of outputs
#define NVOICES  8       //max polyphony
#define SILENCE 0.0003f  //voice choking

class mdaDX10Program
{
  friend class mdaDX10;
public:
  mdaDX10Program() {}
 ~mdaDX10Program() {}

private:
  float param[NPARAMS];
  char  name[24];
};


struct VOICE  //voice state
{
  float env;  //carrier envelope
  float dmod; //modulator oscillator 
  float mod0;
  float mod1;
  float menv; //modulator envelope
  float mlev; //modulator target level
  float mdec; //modulator envelope decay
  float car;  //carrier oscillator
  float dcar;
  float cenv; //smoothed env
  float catt; //smoothing
  float cdec; //carrier envelope decay
  long  note; //remember what note triggered this
};


class mdaDX10 : public AudioEffectX
{
public:
	mdaDX10(audioMasterCallback audioMaster);
	~mdaDX10();

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

	virtual bool getOutputProperties (LvzInt32 index, LvzPinProperties* properties);
	virtual bool getProgramNameIndexed (LvzInt32 category, LvzInt32 index, char* text);
	virtual bool copyProgram (LvzInt32 destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual LvzInt32 getVendorVersion () {return 1;}
	virtual LvzInt32 canDo (char* text);

	virtual LvzInt32 getNumMidiInputChannels ()  { return 1; }

private:
	void update();  //my parameter update
  void noteOn(long note, long velocity);
  void fillpatch(long p, const char *name, 
                 float p0,  float p1,  float p2,  float p3,  float p4,  float p5, 
                 float p6,  float p7,  float p8,  float p9,  float p10, float p11, 
                 float p12, float p13, float p14, float p15);

  float param[NPARAMS];
  mdaDX10Program* programs;
  float Fs;

  #define EVENTBUFFER 120
  #define EVENTS_DONE 99999999
  long notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

  ///global internal variables
  VOICE voice[NVOICES];
  #define SUSTAIN 128
  long sustain, activevoices, K;

  float tune, rati, ratf, ratio; //modulator ratio
  float catt, cdec, crel;        //carrier envelope
  float depth, dept2, mdec, mrel; //modulator envelope
  float lfo0, lfo1, dlfo, modwhl, MW, pbend, velsens, volume, vibrato; //LFO and CC
  float rich, modmix;
};

#endif
