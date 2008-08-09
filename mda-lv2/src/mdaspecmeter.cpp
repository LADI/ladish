//
// Plug-in: "mda Template" v1.0
//
// Copyright(c)2002 Paul Kellett (maxim digital audio)
//


#include <stdio.h> 
#include <string.h>
#include <float.h>
#include <math.h>

#include "mdaSpecMeter.h"
#include "mdaSpecMeterGUI.h"
//#include "AEffEditor.hpp"

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaSpecMeter(audioMaster);
}

mdaSpecMeterProgram::mdaSpecMeterProgram()
{
	param[_PARAM0] = 0.5;
	param[_PARAM1] = 0.5;
	param[_PARAM2] = 0.75;
	strcpy (name, "default");
}


mdaSpecMeter::mdaSpecMeter(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 1, NPARAMS)
{
  editor = new mdaSpecMeterGUI(this);

	programs = new mdaSpecMeterProgram[numPrograms];
	if(programs)
	{
		setProgram(0);
  }

	setNumInputs(2);
	setNumOutputs(2);
	DECLARE_LVZ_DEPRECATED(canMono) ();				      
	setUniqueID("mdaSpecMeter");
	canProcessReplacing();

  //initialise...
  K = counter = 0;
  kmax = 2048;
  topband = 11;
  iK = 1.0f / (float)kmax;
  den = 1.0e-8f;
    
	//buffer = new float[44100];
	
	suspend();
}

bool  mdaSpecMeter::getProductString(char* text) { strcpy(text, "mda SpecMeter"); return true; }
bool  mdaSpecMeter::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaSpecMeter::getEffectName(char* name)    { strcpy(name, "SpecMeter"); return true; }

void mdaSpecMeter::suspend()
{
	Lpeak = Rpeak = Lrms = Rrms = Corr = 0.0f;
  lpeak = rpeak = lrms = rrms = corr = 0.0f;
  Lhold = Rhold = 0.0f;
  Lmin = Rmin = 0.0000001f;
  for(long i=0; i<16; i++) 
  {
    band[0][i] = band[1][i] = 0.0f;
    for(long j=0; j<6; j++) lpp[j][i] = rpp[j][i] = 0.0f;
  }

  //memset(buffer, 0, size * sizeof (float));
}

void mdaSpecMeter::setSampleRate(float sampleRate)
{
  AudioEffectX::setSampleRate(sampleRate);
  if(sampleRate > 64000) { topband = 12;  kmax = 4096; }
                    else { topband = 11;  kmax = 2048; }
  iK = 1.0f / (float)kmax; 
}


mdaSpecMeter::~mdaSpecMeter()
{
	//if(buffer) delete [] buffer;
	if(programs) delete [] programs;
}


void mdaSpecMeter::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void mdaSpecMeter::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
float mdaSpecMeter::getParameter(LvzInt32 index)  { return param[index]; }

void mdaSpecMeter::setProgram(LvzInt32 program)
{
	mdaSpecMeterProgram *p = &programs[program];
	curProgram = program;
  setProgramName(p->name);
	for(long i=0; i<NPARAMS; i++) setParameter(i, p->param[i]);
}

//////////////////////////////////////////////////////////////////////////////////

void mdaSpecMeter::setParameter(LvzInt32 index, float value)
{
	mdaSpecMeterProgram *p = &programs[curProgram];
  param[index] = p->param[index] = value;

	switch(index)
	{
		case _PARAM0: gain = (float)pow(10.0f, 2.0f * param[index] - 1.0f); break;
		
		default: break;
	}
	
	//if(editor) editor->postUpdate();
}


void mdaSpecMeter::getParameterName(LvzInt32 index, char *label)
{
	switch (index)
	{
		case _PARAM0: strcpy (label, ""); break;
		default     : strcpy (label, "");
	}
}


void mdaSpecMeter::getParameterDisplay(LvzInt32 index, char *text)
{
  char string[16];
  
	switch (index)
	{
		case _PARAM0: sprintf(string, "%.1f", 40.0f * param[index] - 20.0f); break;
		case _PARAM1: strcpy (string, ""); break;
		default     : sprintf(string, "%.0f", 100.0f * param[index]);
	}
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaSpecMeter::getParameterLabel(LvzInt32 index, char *label)
{
	switch (index)
	{
		case _PARAM0: strcpy(label, ""); break;
		default     : strcpy(label, "");
	}
}

//////////////////////////////////////////////////////////////////////////////////

void mdaSpecMeter::process (float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1  = inputs[0];
  float *in2  = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];

  den = -den;
  float l, r, p, q, iN = iK;
  long k=K, j0=topband, mask, j;

  while(--sampleFrames >= 0)
  {
    l = *in1++;
    r = *in2++;
    *out1++ += l;
    *out2++ += r;

    l += den; //anti-denormal
    r += den;
    
    lrms += l * l; //RMS integrate
    rrms += r * r;

    p = (float)fabs(l); if(p > lpeak) lpeak = p; //peak detect
    q = (float)fabs(r); if(q > rpeak) rpeak = q;
/*
    if(p > 1.0e-8f && p < lmin) lmin = p; //'trough' detect
    if(q > 1.0e-8f && q < rmin) rmin = q;
*/
    if((l * r) > 0.0f) corr += iN; //measure correlation

    j = j0;
    mask = k << 1;
    
    do //polyphase filter bank
    {
      p = lpp[0][j] + 0.208f * l;
          lpp[0][j] = lpp[1][j];
          lpp[1][j] = l - 0.208f * p;

      q = lpp[2][j] + lpp[4][j] * 0.682f;
          lpp[2][j] = lpp[3][j];
          lpp[3][j] = lpp[4][j] - 0.682f * q;
          lpp[4][j] = l;
          lpp[5][j] += (float)fabs(p - q); //top octave
      l = p + q;                    //lower octaves

      p = rpp[0][j] + 0.208f * r;
          rpp[0][j] = rpp[1][j];
          rpp[1][j] = r - 0.208f * p;

      q = rpp[2][j] + rpp[4][j] * 0.682f;
          rpp[2][j] = rpp[3][j];
          rpp[3][j] = rpp[4][j] - 0.682f * q;
          rpp[4][j] = r;
          rpp[5][j] += (float)fabs(p - q); //top octave
      r = p + q;                    //lower octaves

      j--;
      mask >>= 1;
    } while(mask & 1);
    
    if(++k == kmax)
    {
      k = 0;
      counter++; //editor waits for this to change

      if(lpeak == 0.0f) Lpeak = Lrms = 0.0f; else ///add limits here!
      {
        if(lpeak > 2.0f) lpeak = 2.0f;
        if(lpeak >= Lpeak) 
        {
          Lpeak = lpeak;
          Lhold = 2.0f * Lpeak; 
        }
        else 
        {
          Lhold *= 0.95f;
          if(Lhold < Lpeak) Lpeak = Lhold;
        }
        Lmin = lmin;
        lmin *= 1.01f;
        Lrms += 0.2f * (iN * lrms - Lrms);
      }
      
      if(rpeak == 0.0f) Rpeak = Rrms = 0.0f; else
      {
        if(rpeak > 2.0f) rpeak = 2.0f;
        if(rpeak >= Rpeak) 
        {
          Rpeak = rpeak;
          Rhold = 2.0f * Rpeak; 
        }
        else 
        {
          Rhold *= 0.95f;
          if(Rhold < Rpeak) Rpeak = Rhold;
        }
        Rmin = rmin;
        rmin *= 1.01f;
        Rrms += 0.2f * (iN * rrms - Rrms);
      }
      
      rpeak = lpeak = lrms = rrms = 0.0f;
      Corr += 0.1f * (corr - Corr); //correlation
      corr = SILENCE;

      float dec = 0.08f;
      for(j=0; j<13; j++) //spectrum output
      {
        band[0][j] += dec * (iN * lpp[5][j] - band[0][j]);
        if(band[0][j] > 2.0f) band[0][j] = 2.0f; 
        else if(band[0][j] < 0.014f) band[0][j] = 0.014f;
        
        band[1][j] += dec * (iN * rpp[5][j] - band[1][j]);
        if(band[1][j] > 2.0f) band[1][j] = 2.0f; 
        else if(band[1][j] < 0.014f) band[1][j] = 0.014f;

        rpp[5][j] = lpp[5][j] = SILENCE;
        dec = dec * 1.1f;
      }
    }
  }
  
  K = k;
}

//////////////////////////////////////////////////////////////////////////////////

void mdaSpecMeter::processReplacing (float **inputs, float **outputs, LvzInt32 sampleFrames)
{
  float *in1  = inputs[0];
  float *in2  = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];

  den = -den;
  float l, r, p, q, iN = iK;
  long k=K, j0=topband, mask, j;

  while(--sampleFrames >= 0)
  {
    l = *in1++;
    r = *in2++;
    *out1++ = l;
    *out2++ = r;

    l += den; //anti-denormal
    r += den;
    
    lrms += l * l; //RMS integrate
    rrms += r * r;

    p = (float)fabs(l); if(p > lpeak) lpeak = p; //peak detect
    q = (float)fabs(r); if(q > rpeak) rpeak = q;
/*
    if(p > 1.0e-8f && p < lmin) lmin = p; //'trough' detect
    if(q > 1.0e-8f && q < rmin) rmin = q;
*/
    if((l * r) > 0.0f) corr += iN; //measure correlation

    j = j0;
    mask = k << 1;
    
    do //polyphase filter bank
    {
      p = lpp[0][j] + 0.208f * l;
          lpp[0][j] = lpp[1][j];
          lpp[1][j] = l - 0.208f * p;

      q = lpp[2][j] + lpp[4][j] * 0.682f;
          lpp[2][j] = lpp[3][j];
          lpp[3][j] = lpp[4][j] - 0.682f * q;
          lpp[4][j] = l;
          lpp[5][j] += (float)fabs(p - q); //top octave
      l = p + q;                    //lower octaves

      p = rpp[0][j] + 0.208f * r;
          rpp[0][j] = rpp[1][j];
          rpp[1][j] = r - 0.208f * p;

      q = rpp[2][j] + rpp[4][j] * 0.682f;
          rpp[2][j] = rpp[3][j];
          rpp[3][j] = rpp[4][j] - 0.682f * q;
          rpp[4][j] = r;
          rpp[5][j] += (float)fabs(p - q); //top octave
      r = p + q;                    //lower octaves

      j--;
      mask >>= 1;
    } while(mask & 1);
    
    if(++k == kmax)
    {
      k = 0;
      counter++; //editor waits for this to change

      if(lpeak == 0.0f) Lpeak = Lrms = 0.0f; else ///add limits here!
      {
        if(lpeak > 2.0f) lpeak = 2.0f;
        if(lpeak >= Lpeak) 
        {
          Lpeak = lpeak;
          Lhold = 2.0f * Lpeak; 
        }
        else 
        {
          Lhold *= 0.95f;
          if(Lhold < Lpeak) Lpeak = Lhold;
        }
        Lmin = lmin;
        lmin *= 1.01f;
        Lrms += 0.2f * (iN * lrms - Lrms);
      }
      
      if(rpeak == 0.0f) Rpeak = Rrms = 0.0f; else
      {
        if(rpeak > 2.0f) rpeak = 2.0f;
        if(rpeak >= Rpeak) 
        {
          Rpeak = rpeak;
          Rhold = 2.0f * Rpeak; 
        }
        else 
        {
          Rhold *= 0.95f;
          if(Rhold < Rpeak) Rpeak = Rhold;
        }
        Rmin = rmin;
        rmin *= 1.01f;
        Rrms += 0.2f * (iN * rrms - Rrms);
      }
      
      rpeak = lpeak = lrms = rrms = 0.0f;
      Corr += 0.1f * (corr - Corr); //correlation
      corr = SILENCE;

      float dec = 0.08f;
      for(j=0; j<13; j++) //spectrum output
      {
        band[0][j] += dec * (iN * lpp[5][j] - band[0][j]);
        if(band[0][j] > 2.0f) band[0][j] = 2.0f; 
        else if(band[0][j] < 0.014f) band[0][j] = 0.014f;
        
        band[1][j] += dec * (iN * rpp[5][j] - band[1][j]);
        if(band[1][j] > 2.0f) band[1][j] = 2.0f; 
        else if(band[1][j] < 0.014f) band[1][j] = 0.014f;

        rpp[5][j] = lpp[5][j] = SILENCE;
        dec = dec * 1.1f;
      }
    }
  }
  
  K = k;
}
