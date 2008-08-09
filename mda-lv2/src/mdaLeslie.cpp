//Remove optimization /Og else wavelab crashes!!!

#include "mdaLeslie.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaLeslie(audioMaster);
}

mdaLeslieProgram::mdaLeslieProgram()
{
	fParam1 = 0.66f;
  fParam7 = 0.50f;
  fParam9 = 0.60f;
  fParam4 = 0.70f;
  fParam5 = 0.60f;
  fParam6 = 0.70f;
  fParam3 = 0.48f;
  fParam2 = 0.50f;
  fParam8 = 0.50f;
  strcpy(name, "Leslie Simulator");
}

mdaLeslie::mdaLeslie(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 3, 9)	// programs, parameters 
{
	fParam1 = 0.66f;
  fParam7 = 0.50f;
  fParam9 = 0.60f;
  fParam4 = 0.70f;
  fParam5 = 0.60f;
  fParam6 = 0.70f;
  fParam3 = 0.48f;
  fParam2 = 0.50f;
  fParam8 = 0.50f;

  size = 256; hpos = 0;
	hbuf = new float[size];
  fbuf1 = fbuf2 = 0.0f;
  twopi = 6.2831853f;
  
  setNumInputs(2);		  
	setNumOutputs(2);		  
	setUniqueID("mdaLeslie");  // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();				      
	canProcessReplacing();	
	suspend();		

  programs = new mdaLeslieProgram[numPrograms]; 
  if(programs) 
  {
    programs[1].fParam1 = 0.33f;
    programs[1].fParam5 = 0.75f;
    programs[1].fParam6 = 0.57f;
    strcpy(programs[1].name,"Slow");
    programs[2].fParam1 = 0.66f;
    programs[2].fParam5 = 0.60f;
    programs[2].fParam6 = 0.70f;
    strcpy(programs[2].name,"Fast");
    setProgram(0);
  }

  chp = dchp = clp = dclp = shp = dshp = slp = dslp = 0.0f;

  lspd = 0.0f; hspd = 0.0f;
  lphi = 0.0f; hphi = 1.6f; 
  setParameter(0, 0.66f);
}

bool  mdaLeslie::getProductString(char* text) { strcpy(text, "mda Leslie"); return true; }
bool  mdaLeslie::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaLeslie::getEffectName(char* name)    { strcpy(name, "Leslie"); return true; }

void mdaLeslie::setParameter(LvzInt32 index, float value)
{
  
  float ifs = 1.0f / getSampleRate();
  float spd = twopi * ifs * 2.0f * fParam8;

  switch(index)
  {
    case 0: programs[curProgram].fParam1 = fParam1 = value; break;
    case 1: programs[curProgram].fParam7 = fParam7 = value; break;
    case 2: programs[curProgram].fParam9 = fParam9 = value; break;
    case 3: programs[curProgram].fParam4 = fParam4 = value; break;
    case 4: programs[curProgram].fParam5 = fParam5 = value; break;
    case 5: programs[curProgram].fParam6 = fParam6 = value; break;
    case 6: programs[curProgram].fParam3 = fParam3 = value; break;
    case 7: programs[curProgram].fParam2 = fParam2 = value; break;
    case 8: programs[curProgram].fParam8 = fParam8 = value; break;
  }
  //calcs here!
  filo = 1.f - (float)pow(10.0f, fParam3 * (2.27f - 0.54f * fParam3) - 1.92f);

  if(fParam1<0.50f)
  {  
     if(fParam1<0.1f) //stop
     { 
       lset = 0.00f; hset = 0.00f;
       lmom = 0.12f; hmom = 0.10f; 
     }
     else //low speed
     { 
       lset = 0.49f; hset = 0.66f;
       lmom = 0.27f; hmom = 0.18f;
     }
  }
  else //high speed
  {  
    lset = 5.31f; hset = 6.40f;
    lmom = 0.14f; hmom = 0.09f;
  }
  hmom = (float)pow(10.0f, -ifs / hmom);
  lmom = (float)pow(10.0f, -ifs / lmom); 
  hset *= spd;
  lset *= spd;

  gain = 0.4f * (float)pow(10.0f, 2.0f * fParam2 - 1.0f);
  lwid = fParam7 * fParam7;
  llev = gain * 0.9f * fParam9 * fParam9;
  hwid = fParam4 * fParam4;
  hdep = fParam5 * fParam5 * getSampleRate() / 760.0f;
  hlev = gain * 0.9f * fParam6 * fParam6;
}

mdaLeslie::~mdaLeslie()
{
  if(hbuf) delete [] hbuf;
  if(programs) delete [] programs; 
}

void mdaLeslie::setProgram(LvzInt32 program) 
{
	mdaLeslieProgram *p = &programs[program];

	curProgram = program;
	setParameter(0, p->fParam1);	
	setParameter(1, p->fParam7);	
	setParameter(2, p->fParam9);	
	setParameter(3, p->fParam4);	
	setParameter(4, p->fParam5);	
	setParameter(5, p->fParam6);
	setParameter(6, p->fParam3);	
	setParameter(7, p->fParam2);
	setParameter(8, p->fParam8);	
  setProgramName(p->name);
}

void mdaLeslie::suspend()
{
	memset(hbuf, 0, size * sizeof(float));
}

void mdaLeslie::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaLeslie::getProgramName(char *name)
{
	strcpy(name, programName);
}

float mdaLeslie::getParameter(LvzInt32 index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam7; break;
    case 2: v = fParam9; break;
    case 3: v = fParam4; break;
    case 4: v = fParam5; break;
    case 5: v = fParam6; break;
    case 6: v = fParam3; break;
    case 7: v = fParam2; break;
    case 8: v = fParam8; break;
  }
  return v;
}

void mdaLeslie::getParameterName(LvzInt32 index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Speed:"); break;
    case 1: strcpy(label, "Lo Width"); break;
    case 2: strcpy(label, "Lo Throb"); break;
    case 3: strcpy(label, "Hi Width"); break;
    case 4: strcpy(label, "Hi Depth"); break;
    case 5: strcpy(label, "Hi Throb"); break;
    case 6: strcpy(label, "X-Over"); break;
    case 7: strcpy(label, "Output"); break;
    case 8: strcpy(label, "Speed "); break;
  }
}

#include <stdio.h>
void long2string(long value, char *string) { sprintf(string, "%ld", value); }

void mdaLeslie::getParameterDisplay(LvzInt32 index, char *text)
{
	switch(index)
  {
    case 0: 
      if(fParam1<0.5f) 
      { 
        if(fParam1 < 0.1f) strcpy(text, "STOP"); 
                      else strcpy(text, "SLOW");
      }               else strcpy(text, "FAST");  break;   
    case 1: long2string((long)(100 * fParam7), text); break;
    case 2: long2string((long)(100 * fParam9), text); break;
    case 3: long2string((long)(100 * fParam4), text); break;
    case 4: long2string((long)(100 * fParam5), text); break;
    case 5: long2string((long)(100 * fParam6), text); break;
    case 6: long2string((long)(10*int((float)pow(10.0f,1.179f + fParam3))), text); break; 
    case 7: long2string((long)(40 * fParam2 - 20), text); break;
    case 8: long2string((long)(200 * fParam8), text); break;
  }
}

void mdaLeslie::getParameterLabel(LvzInt32 index, char *label)
{
	switch(index)
  {
    case  0: strcpy(label, ""); break;
    case  6: strcpy(label, "Hz"); break;
    case  7: strcpy(label, "dB"); break;
    default: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------

void mdaLeslie::process(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, g=gain, h, l;	
  float fo=filo, fb1=fbuf1, fb2=fbuf2;
  float hl=hlev, hs=hspd, ht, hm=hmom, hp=hphi, hw=hwid, hd=hdep;
  float ll=llev, ls=lspd, lt, lm=lmom, lp=lphi, lw=lwid;
  float hint, k0=0.03125f, k1=32.f;
  long  hdd, hdd2, k=0, hps=hpos;

  ht=hset*(1.f-hm);
  lt=lset*(1.f-lm);

  chp = (float)cos(hp); chp *= chp * chp;
  clp = (float)cos(lp);
  shp = (float)sin(hp);
  slp = (float)sin(lp);

  --in1;	
	--in2;	
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2; 
    c = out1[1];
		d = out2[1]; //see processReplacing() for comments

    if(k) k--; else 
    {
      ls = (lm * ls) + lt; 
      hs = (hm * hs) + ht;
      lp += k1 * ls;
      hp += k1 * hs;
                          
      dchp = (float)cos(hp + k1*hs);
      dchp = k0 * (dchp * dchp * dchp - chp); 
      dclp = k0 * ((float)cos(lp + k1*ls) - clp);
      dshp = k0 * ((float)sin(hp + k1*hs) - shp);
      dslp = k0 * ((float)sin(lp + k1*ls) - slp);
      
      k=(long)k1;
    }

    fb1 = fo * (fb1 - a) + a;
    fb2 = fo * (fb2 - fb1) + fb1;  
    h = (g - hl * chp) * (a - fb2);
    l = (g - ll * clp) * fb2;

    if(hps>0) hps--; else hps=200; 
    hint = hps + hd * (1.0f + chp);
    hdd = (int)hint; 
    hint = hint - hdd;
    hdd2 = hdd + 1;
    if(hdd>199) { if(hdd>200) hdd -= 201; hdd2 -= 201; }

    *(hbuf + hps) = h;
    a = *(hbuf + hdd);
    h += a + hint * ( *(hbuf + hdd2) - a);

    c += l + h;
    d += l + h;
    h *= hw * shp;
    l *= lw * slp;
    d += l - h;
    c += h - l;

    *++out1 = c;
		*++out2 = d;

    chp += dchp;
    clp += dclp;
    shp += dshp;
    slp += dslp;
	}
  lspd = ls;
  hspd = hs;
  hpos = hps;
  lphi = (float)fmod(lp+(k1-k)*ls,twopi);
  hphi = (float)fmod(hp+(k1-k)*hs,twopi);
  if(fabs(fb1)>1.0e-10) fbuf1=fb1; else fbuf1=0.f; 
  if(fabs(fb2)>1.0e-10) fbuf2=fb2; else fbuf2=0.f; 
}

void mdaLeslie::processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, g=gain, h, l;	
  float fo=filo, fb1=fbuf1, fb2=fbuf2;
  float hl=hlev, hs=hspd, ht, hm=hmom, hp=hphi, hw=hwid, hd=hdep;
  float ll=llev, ls=lspd, lt, lm=lmom, lp=lphi, lw=lwid;
  float hint, k0=0.03125f, k1=32.f; //k0 = 1/k1
  long  hdd, hdd2, k=0, hps=hpos;

  ht=hset*(1.f-hm); //target speeds
  lt=lset*(1.f-lm);

  chp = (float)cos(hp); chp *= chp * chp; //set LFO values
  clp = (float)cos(lp);
  shp = (float)sin(hp);
  slp = (float)sin(lp);

	--in1;	
	--in2;	
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2; //mono input

    if(k) k--; else //linear piecewise approx to LFO waveforms
    {
      ls = (lm * ls) + lt; //tend to required speed
      hs = (hm * hs) + ht;
      lp += k1 * ls;
      hp += k1 * hs;
                           
      dchp = (float)cos(hp + k1*hs);
      dchp = k0 * (dchp * dchp * dchp - chp); //sin^3 level mod
      dclp = k0 * ((float)cos(lp + k1*ls) - clp);
      dshp = k0 * ((float)sin(hp + k1*hs) - shp);
      dslp = k0 * ((float)sin(lp + k1*ls) - slp);
      
      k=(long)k1;
    }

    fb1 = fo * (fb1 - a) + a; //crossover
    fb2 = fo * (fb2 - fb1) + fb1;  
    h = (g - hl * chp) * (a - fb2); //volume
    l = (g - ll * clp) * fb2;

    if(hps>0) hps--; else hps=200;  //delay input pos
    hint = hps + hd * (1.0f + chp); //delay output pos 
    hdd = (int)hint; 
    hint = hint - hdd; //linear intrpolation
    hdd2 = hdd + 1;
    if(hdd>199) { if(hdd>200) hdd -= 201; hdd2 -= 201; }

    *(hbuf + hps) = h; //delay input
    a = *(hbuf + hdd);
    h += a + hint * ( *(hbuf + hdd2) - a); //delay output

    c = l + h; 
    d = l + h;
    h *= hw * shp;
    l *= lw * slp;
    d += l - h;
    c += h - l;

    *++out1 = c; //output
		*++out2 = d;

    chp += dchp;
    clp += dclp;
    shp += dshp;
    slp += dslp;
  }
  lspd = ls;
  hspd = hs;
  hpos = hps;
  lphi = (float)fmod(lp+(k1-k)*ls,twopi);
  hphi = (float)fmod(hp+(k1-k)*hs,twopi);
  if(fabs(fb1)>1.0e-10) fbuf1=fb1; else fbuf1=0.0f; //catch denormals
  if(fabs(fb2)>1.0e-10) fbuf2=fb2; else fbuf2=0.0f; 
}
