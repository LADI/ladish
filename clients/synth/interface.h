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

#ifndef __SYNTH_INTERFACE_H__
#define __SYNTH_INTERFACE_H__

#include "config.h"

#ifdef HAVE_GTK2

#include <gtk/gtk.h>

extern GtkWidget * modulation_range;
extern GtkWidget * attack_range;
extern GtkWidget * decay_range;
extern GtkWidget * sustain_range;
extern GtkWidget * release_range;
extern GtkWidget * gain_range;
extern GtkWidget * harmonic_spin;
extern GtkWidget * subharmonic_spin;
extern GtkWidget * transpose_spin;

extern int interface_up;

void * interface_main (void *);

#endif /* HAVE_GTK2 */

#endif /* __SYNTH_INTERFACE_H__ */
