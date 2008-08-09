//
// Plug-in: "mda Template" v1.0
//
// Copyright(c)1999-2000 Paul Kellett (maxim digital audio)
//

#include "mdaTalkBox.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaTalkBox(audioMaster);
}

mdaTalkBoxProgram::mdaTalkBoxProgram() ///default program settings
{
  param[0] = 0.5f;  //wet
  param[1] = 0.0f;  //dry
  param[2] = 0.0f;  //swap
  param[3] = 1.0f;  //quality
  strcpy(name, "Talkbox");
}


mdaTalkBox::mdaTalkBox(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaTalkBox");  ///identify plug-in here
  //canMono();
  canProcessReplacing();

  ///initialise...
  buf0 = new float[BUF_MAX];
  buf1 = new float[BUF_MAX];
  window = new float[BUF_MAX];
  car0 = new float[BUF_MAX];
  car1 = new float[BUF_MAX];
  N = 1; //trigger window recalc
  K = 0;

  programs = new mdaTalkBoxProgram[numPrograms];
  if(programs)
  {
    ///differences from default program...
    //programs[1].param[0] = 0.1f;
    //strcpy(programs[1].name,"Another Program");

    setProgram(0);
  }
  
  suspend();
}

bool  mdaTalkBox::getProductString(char* text) { strcpy(text, "mda TalkBox"); return true; }
bool  mdaTalkBox::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaTalkBox::getEffectName(char* name)    { strcpy(name, "TalkBox"); return true; }

void mdaTalkBox::resume() ///update internal parameters...
{
  float fs = getSampleRate();
  if(fs <  8000.0f) fs =  8000.0f;
  if(fs > 96000.0f) fs = 96000.0f;
  
  swap = (param[2] > 0.5f)? 1 : 0;

  long n = (long)(0.01633f * fs);
  if(n > BUF_MAX) n = BUF_MAX;
  
  //O = (long)(0.0005f * fs);
  O = (long)((0.0001f + 0.0004f * param[3]) * fs);

  if(n != N) //recalc hanning window
  {
    N = n;
    float dp = TWO_PI / (float)N;
    float p = 0.0f;
    for(n=0; n<N; n++)
    {
      window[n] = 0.5f - 0.5f * (float)cos(p);
      p += dp;
    }
  }
  wet = 0.5f * param[0] * param[0];
  dry = 2.0f * param[1] * param[1];
}


void mdaTalkBox::suspend() ///clear any buffers...
{
  pos = K = 0;
  emphasis = 0.0f;
  FX = 0;

  u0 = u1 = u2 = u3 = u4 = 0.0f;
  d0 = d1 = d2 = d3 = d4 = 0.0f;

  memset(buf0, 0, BUF_MAX * sizeof(float));
  memset(buf1, 0, BUF_MAX * sizeof(float));
  memset(car0, 0, BUF_MAX * sizeof(float));
  memset(car1, 0, BUF_MAX * sizeof(float));
}


mdaTalkBox::~mdaTalkBox() ///destroy any buffers...
{
  if(buf0) delete [] buf0;
  if(buf1) delete [] buf1;
  if(window) delete [] window;
  if(car0) delete [] car0;
  if(car1) delete [] car1;
  if(programs) delete[] programs;
}


void mdaTalkBox::setProgram(LvzInt32 program)
{
  int i=0;

  mdaTalkBoxProgram *p = &programs[program];
  curProgram = program;
  for(i=0; i<NPARAMS; i++) param[i] = p->param[i];
  resume();
}


void  mdaTalkBox::setParameter(LvzInt32 index, float value) 
{ 
  programs[curProgram].param[index] = param[index] = value; //bug was here!
  resume(); 
}

float mdaTalkBox::getParameter(LvzInt32 index) { return param[index]; }
void  mdaTalkBox::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaTalkBox::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }


void mdaTalkBox::getParameterName(LvzInt32 index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Wet"); break;
    case  1: strcpy(label, "Dry"); break;
    case  2: strcpy(label, "Carrier:"); break;
    case  3: strcpy(label, "Quality"); break;
    default: strcpy(label, "");
  }
}


void mdaTalkBox::getParameterDisplay(LvzInt32 index, char *text)
{
 	char string[16];

  switch(index)
  {
    case 2: if(swap) strcpy(string, "LEFT"); else strcpy(string, "RIGHT"); break;
    
    case 3: sprintf(string, "%4.0f", 5.0f + 95.0f * param[index] * param[index]); break;

    default: sprintf(string, "%4.0f %%", 200.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaTalkBox::getParameterLabel(LvzInt32 index, char *label)
{
  switch(index)
  {
    case  0:
    case  1: strcpy(label, ""); break;

    default: strcpy(label, "");
  }
}


void mdaTalkBox::process(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  if(swap)
  {
    in1 = inputs[1];
    in2 = inputs[0];
  }
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  long  p0=pos, p1 = (pos + N/2) % N;
  float e=emphasis, w, o, x, c, d, dr, fx=FX;
  float p, q, h0=0.3f, h1=0.77f;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    o = *++in1;
    x = *++in2;
    c = out1[1];
    d = out2[1];

    dr = o;

    p = d0 + h0 *  x; d0 = d1;  d1 = x  - h0 * p;
    q = d2 + h1 * d4; d2 = d3;  d3 = d4 - h1 * q;  
    d4 = x;
    x = p + q;

    if(K++)
    {
      K = 0;

      car0[p0] = car1[p1] = x; //carrier input
    
      x = o - e;  e = o;  //6dB/oct pre-emphasis

      w = window[p0]; fx = buf0[p0] * w;  buf0[p0] = x * w;  //50% overlapping hanning windows
      if(++p0 >= N) { lpc(buf0, car0, N, O);  p0 = 0; }

      w = 1.0f - w;  fx += buf1[p1] * w;  buf1[p1] = x * w;
      if(++p1 >= N) { lpc(buf1, car1, N, O);  p1 = 0; }
    }

    p = u0 + h0 * fx; u0 = u1;  u1 = fx - h0 * p;
    q = u2 + h1 * u4; u2 = u3;  u3 = u4 - h1 * q;  
    u4 = fx;
    x = p + q;

    o = wet * x + dry * dr;
    *++out1 = c + o;
    *++out2 = d + o;
  }
  emphasis = e;
  pos = p0;
  FX = fx;

  float den = 1.0e-10f; //(float)pow(10.0f, -10.0f * param[4]);
  if(fabs(d0) < den) d0 = 0.0f; //anti-denormal (doesn't seem necessary but P4?)
  if(fabs(d1) < den) d1 = 0.0f;
  if(fabs(d2) < den) d2 = 0.0f;
  if(fabs(d3) < den) d3 = 0.0f;
  if(fabs(u0) < den) u0 = 0.0f;
  if(fabs(u1) < den) u1 = 0.0f;
  if(fabs(u2) < den) u2 = 0.0f;
  if(fabs(u3) < den) u3 = 0.0f;
}


void mdaTalkBox::processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  if(swap)
  {
    in1 = inputs[1];
    in2 = inputs[0];
  }
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  long  p0=pos, p1 = (pos + N/2) % N;
  float e=emphasis, w, o, x, dr, fx=FX;
  float p, q, h0=0.3f, h1=0.77f;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    o = *++in1;
    x = *++in2;
    dr = o;

    p = d0 + h0 *  x; d0 = d1;  d1 = x  - h0 * p;
    q = d2 + h1 * d4; d2 = d3;  d3 = d4 - h1 * q;  
    d4 = x;
    x = p + q;

    if(K++)
    {
      K = 0;

      car0[p0] = car1[p1] = x; //carrier input
    
      x = o - e;  e = o;  //6dB/oct pre-emphasis

      w = window[p0]; fx = buf0[p0] * w;  buf0[p0] = x * w;  //50% overlapping hanning windows
      if(++p0 >= N) { lpc(buf0, car0, N, O);  p0 = 0; }

      w = 1.0f - w;  fx += buf1[p1] * w;  buf1[p1] = x * w;
      if(++p1 >= N) { lpc(buf1, car1, N, O);  p1 = 0; }
    }

    p = u0 + h0 * fx; u0 = u1;  u1 = fx - h0 * p;
    q = u2 + h1 * u4; u2 = u3;  u3 = u4 - h1 * q;  
    u4 = fx;
    x = p + q;

    o = wet * x + dry * dr;
    *++out1 = o;
    *++out2 = o;
  }
  emphasis = e;
  pos = p0;
  FX = fx;

  float den = 1.0e-10f; //(float)pow(10.0f, -10.0f * param[4]);
  if(fabs(d0) < den) d0 = 0.0f; //anti-denormal (doesn't seem necessary but P4?)
  if(fabs(d1) < den) d1 = 0.0f;
  if(fabs(d2) < den) d2 = 0.0f;
  if(fabs(d3) < den) d3 = 0.0f;
  if(fabs(u0) < den) u0 = 0.0f;
  if(fabs(u1) < den) u1 = 0.0f;
  if(fabs(u2) < den) u2 = 0.0f;
  if(fabs(u3) < den) u3 = 0.0f;
}


void mdaTalkBox::lpc(float *buf, float *car, long n, long o)
{
  float z[ORD_MAX], r[ORD_MAX], k[ORD_MAX], G, x;
  long i, j, nn=n;

  for(j=0; j<=o; j++, nn--)  //buf[] is already emphasized and windowed
  {
    z[j] = r[j] = 0.0f;
    for(i=0; i<nn; i++) r[j] += buf[i] * buf[i+j]; //autocorrelation
  }
  r[0] *= 1.001f;  //stability fix

  float min = 0.00001f;
  if(r[0] < min) { for(i=0; i<n; i++) buf[i] = 0.0f; return; } 

  lpc_durbin(r, o, k, &G);  //calc reflection coeffs

  for(i=0; i<=o; i++) 
  {
    if(k[i] > 0.995f) k[i] = 0.995f; else if(k[i] < -0.995f) k[i] = -.995f;
  }
  
  for(i=0; i<n; i++)
  {
    x = G * car[i];
    for(j=o; j>0; j--)  //lattice filter
    { 
      x -= k[j] * z[j-1];
      z[j] = z[j-1] + k[j] * x;
    }
    buf[i] = z[0] = x;  //output buf[] will be windowed elsewhere
  }
}


void mdaTalkBox::lpc_durbin(float *r, int p, float *k, float *g)
{
  int i, j;
  float a[ORD_MAX], at[ORD_MAX], e=r[0];
    
  for(i=0; i<=p; i++) a[i] = at[i] = 0.0f; //probably don't need to clear at[] or k[]

  for(i=1; i<=p; i++) 
  {
    k[i] = -r[i];

    for(j=1; j<i; j++) 
    { 
      at[j] = a[j];
      k[i] -= a[j] * r[i-j]; 
    }
    if(fabs(e) < 1.0e-20f) { e = 0.0f;  break; }
    k[i] /= e;
    
    a[i] = k[i];
    for(j=1; j<i; j++) a[j] = at[j] + k[i] * at[i-j];
    
    e *= 1.0f - k[i] * k[i];
  }

  if(e < 1.0e-20f) e = 0.0f;
  *g = (float)sqrt(e);
}
