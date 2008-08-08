//See associated .cpp file for copyright and other info
#include "audioeffectx.h"

#define NPARAMS            4  ///number of parameters
#define NPROGS             1  ///number of programs
#define BUF_MAX         1600
#define ORD_MAX           50
#define TWO_PI     6.28318530717958647692528676655901f


class mdaTalkBoxProgram
{
public:
  mdaTalkBoxProgram();
  ~mdaTalkBoxProgram() {}

private:
  friend class mdaTalkBox;
  float param[NPARAMS];
  char name[24];
};


class mdaTalkBox : public AudioEffectX
{
public:
  mdaTalkBox(audioMasterCallback audioMaster);
  ~mdaTalkBox();

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
  mdaTalkBoxProgram *programs;

  void lpc(float *buf, float *car, long n, long o);
  void lpc_durbin(float *r, int p, float *k, float *g);

  ///global internal variables
  float *car0, *car1;
  float *window;
  float *buf0, *buf1;
  
  float emphasis;
  long K, N, O, pos, swap;
  float wet, dry, FX;

  float d0, d1, d2, d3, d4;
  float u0, u1, u2, u3, u4;
};


