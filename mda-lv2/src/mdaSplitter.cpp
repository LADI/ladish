//
// mda plug-in: "mda Splitter" v1.0
//
// Copyright(c)1999-2000 Paul Kellett (maxim digital audio)
//

#include "mdaSplitter.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaSplitter(audioMaster);
}

mdaSplitterProgram::mdaSplitterProgram() ///default program settings
{
  param[0] = 0.10f;  //mode
  param[1] = 0.50f;  //freq
  param[2] = 0.25f;  //freq mode
  param[3] = 0.50f;  //level (was 2)
  param[4] = 0.50f;  //level mode
  param[5] = 0.50f;  //envelope
  param[6] = 0.50f;  //gain
  strcpy(name, "Frequency/Level Splitter");
}


mdaSplitter::mdaSplitter(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mda7");  ///identify plug-in here
	DECLARE_LVZ_DEPRECATED(canMono) ();				      
  canProcessReplacing();

  programs = new mdaSplitterProgram[numPrograms];
  setProgram(0);
  
  ///differences from default program...
  programs[1].param[2] = 0.50f;
  programs[1].param[4] = 0.25f;
  strcpy(programs[1].name,"Pass Peaks Only");
  programs[2].param[0] = 0.60f;
  strcpy(programs[2].name,"Stereo Crossover");
  
  suspend();
}

bool  mdaSplitter::getProductString(char* text) { strcpy(text, "mda Splitter"); return true; }
bool  mdaSplitter::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaSplitter::getEffectName(char* name)    { strcpy(name, "Splitter"); return true; }

void mdaSplitter::resume() ///update internal parameters...
{
  long tmp;

  freq = param[1];
  fdisp = (float)pow(10.0f, 2.0f + 2.0f * freq);  //frequency
  freq = 5.5f * fdisp / getSampleRate();
  if(freq>1.0f) freq = 1.0f;

  ff = -1.0f;               //above
  tmp = (long)(2.9f * param[2]);  //frequency switching
  if(tmp==0) ff = 0.0f;     //below
  if(tmp==1) freq = 0.001f; //all

  ldisp = 40.0f * param[3] - 40.0f;  //level
  level = (float)pow(10.0f, 0.05f * ldisp + 0.3f);

  ll = 0.0f;                //above
  tmp = (long)(2.9f * param[4]);  //level switching
  if(tmp==0) ll = -1.0f;    //below
  if(tmp==1) level = 0.0f;  //all
  
  pp = -1.0f;  //phase correction
  if(ff==ll) pp = 1.0f;
  if(ff==0.0f && ll==-1.0f) { ll *= -1.0f; }

  att = 0.05f - 0.05f * param[5];
  rel = 1.0f - (float)exp(-6.0f - 4.0f * param[5]); //envelope
  if(att>0.02f) att=0.02f;
  if(rel<0.9995f) rel = 0.9995f;

  i2l = i2r = o2l = o2r = (float)pow(10.0f, 2.0f * param[6] - 1.0f);  //gain

  mode = (long)(3.9f * param[0]);  //output routing
  switch(mode)
  {
    case  0: i2l  =  0.0f;  i2r  =  0.0f;  break;
    case  1: o2l *= -1.0f;  o2r *= -1.0f;  break;
    case  2: i2l  =  0.0f;  o2r *= -1.0f;  break;
    default: o2l *= -1.0f;  i2r  =  0.0f;  break;
  }
}


void mdaSplitter::suspend() ///clear any buffers...
{
  env = buf0 = buf1 = buf2 = buf3 = 0.0f;
}


mdaSplitter::~mdaSplitter() ///destroy any buffers...
{
  if(programs) delete [] programs;
}


void mdaSplitter::setProgram(LvzInt32 program)
{
  int i=0;

  mdaSplitterProgram *p = &programs[program];
  curProgram = program;
  setProgramName(p->name);
  for(i=0; i<NPARAMS; i++) param[i] = p->param[i];
  resume();
}


void  mdaSplitter::setParameter(LvzInt32 index, float value) 
{ 
  programs[curProgram].param[index] = param[index] = value; //bug was here!
  resume(); 
}


float mdaSplitter::getParameter(LvzInt32 index) { return param[index]; }
void  mdaSplitter::setProgramName(char *name) { strcpy(programName, name); }
void  mdaSplitter::getProgramName(char *name) { strcpy(name, programName); }


void mdaSplitter::getParameterName(LvzInt32 index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Mode"); break; 
    case  1: 
    case  2: strcpy(label, "Freq"); break;
    case  3: 
    case  4: strcpy(label, "Level"); break;
    case  5: strcpy(label, "Envelope"); break;
    default: strcpy(label, "Output");
  }
}


void mdaSplitter::getParameterDisplay(LvzInt32 index, char *text)
{
 	char string[16];

  switch(index)
  {
    case  0: switch(mode)
             {
               case  0:  strcpy (string, "NORMAL "); break;
               case  1:  strcpy (string, "INVERSE "); break;
               case  2:  strcpy (string, "NORM/INV"); break;
               default:  strcpy (string, "INV/NORM"); break;
             } break;
    case  1: sprintf(string, "%.0f", fdisp); break;
    case  3: sprintf(string, "%.0f", ldisp); break;
    case  5: sprintf(string, "%.0f", (float)pow(10.0f, 1.0f + 2.0f * param[index])); break;
    case  6: sprintf(string, "%.1f", 40.0f * param[index] - 20.0f); break;
    default: switch((long)(2.9f * param[index]))
             {
                case  0: strcpy (string, "BELOW"); break;
                case  1: strcpy (string, "ALL"); break;
                default: strcpy (string, "ABOVE"); break;
             } break;
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaSplitter::getParameterLabel(LvzInt32 index, char *label)
{
  switch(index)
  {
    case  1: strcpy(label, "Hz"); break;
    case  3: 
    case  6: strcpy(label, "dB"); break;
    case  5: strcpy(label, "ms"); break;
    default: strcpy(label, "");
  }
}


void mdaSplitter::process(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float a0=buf0, a1=buf1, b0=buf2, b1=buf3, f=freq, fx=ff;
  float aa, bb, ee, e=env, at=att, re=rel, l=level, lx=ll, px=pp;
  float il=i2l, ir=i2r, ol=o2l, or_=o2r;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1;
    b = *++in2;
    c = out1[1];
    d = out2[1];

    a0 += f * (a - a0 - a1);  //frequency split
    a1 += f * a0;
    aa = a1 + fx * a;

    b0 += f * (b - b0 - b1);
    b1 += f * b0;
    bb = b1 + fx * b;

    ee = aa + bb;
    if(ee<0.0f) ee = -ee;
    if(ee>l) e += at * (px - e);  //level split
    e *= re;

    c += il * a + ol * aa * (e + lx);
    d += ir * b + or_ * bb * (e + lx);

    *++out1 = c;
    *++out2 = d;
  }
  env = e;  if(fabs(e)<1.0e-10) env = 0.0f;
  buf0 = a0;  buf1 = a1;  buf2 = b0;  buf3 = b1;
  if(fabs(a0)<1.0e-10) { buf0 = buf1 = buf2 = buf3 = 0.0f; }  //catch denormals
}


void mdaSplitter::processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b;
  float a0=buf0, a1=buf1, b0=buf2, b1=buf3, f=freq, fx=ff;
  float aa, bb, ee, e=env, at=att, re=rel, l=level, lx=ll, px=pp;
  float il=i2l, ir=i2r, ol=o2l, or_=o2r;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1;
    b = *++in2;
    
    a0 += f * (a - a0 - a1);  //frequency split
    a1 += f * a0;
    aa = a1 + fx * a;

    b0 += f * (b - b0 - b1);
    b1 += f * b0;
    bb = b1 + fx * b;

    ee = aa + bb;
    if(ee<0.0f) ee = -ee;
    if(ee>l) e += at * (px - e);  //level split
    e *= re;

    a = il * a + ol * aa * (e + lx);
    b = ir * b + or_ * bb * (e + lx);

    *++out1 = a;
    *++out2 = b;
  }
  env = e;  if(fabs(e)<1.0e-10) env = 0.0f;
  buf0 = a0;  buf1 = a1;  buf2 = b0;  buf3 = b1;
  if(fabs(a0)<1.0e-10) { buf0 = buf1 = buf2 = buf3 = 0.0f; }  //catch denormals
}
