/*
 *   LASH
 *    
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_SYNTH_H__
#define __LASH_SYNTH_H__

#include <lash/lash.h>

extern lash_client_t * lash_client;
extern double modulation, attack, decay, sustain, release, gain, sample_rate;
extern int harmonic, subharmonic, transpose;
extern char jack_client_name[512];
extern char alsa_client_name[512];  
extern int quit;

void * synth_main (void *);

#endif /* __LASH_SYNTH_H__ */
