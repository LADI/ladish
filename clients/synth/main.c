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

#define _GNU_SOURCE

#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
       
#include <lash/lash.h>

#ifdef HAVE_GTK2
#include <gtk/gtk.h>
#include "interface.h"
#endif

#include "synth.h"
#include "lash.h"

#define MODULATION_DEFAULT    0.8
#define HARMONIC_DEFAULT      1
#define SUBHARMONIC_DEFAULT   3
#define TRANSPOSE_DEFAULT     12
#define ATTACK_DEFAULT        0.05
#define DECAY_DEFAULT         0.3
#define SUSTAIN_DEFAULT       0.8
#define RELEASE_DEFAULT       0.2
#define GAIN_DEFAULT          1.0

#define CLIENT_NAME_BASE           "LASH Synth"
#define JACK_CLIENT_NAME_BASE      "lash_synth"

static void print_help () {
  printf("LASH Synth version %s\n", PACKAGE_VERSION);
  printf("Copyright (C) 2002 Robert Ham <rah@bash.sh>\n");
  printf("\n");
  printf("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
  printf("under the terms of the GNU General Public License, version 2 or later.  See\n");
  printf("the COPYING file that came with this software for details.\n");
  printf("\n");
  printf(" -h, --help                  Display this help info\n");
  printf("     --lash-port=<port>      Connect to server on port <port>\n");
  printf(" -p, --pid                   Use the pid in the client string (default)\n");
  printf(" -e, --name <string>         Use the specified string in the client string\n");
  printf(" -b, --basic-name            Use just the program name in the client string\n");
  printf("\n");
  printf(" -m, --modulation <float>    Set the synth modulation parameter  [%g]\n", MODULATION_DEFAULT);
  printf(" -n, --harmonic <int>        Set the synth harmonic parameter    [%d]\n", HARMONIC_DEFAULT);
  printf(" -u, --subharmonic <int>     Set the synth subharmonic parameter [%d]\n", SUBHARMONIC_DEFAULT);
  printf(" -t, --transpose <int>       Set the synth transpose parameter   [%d]\n", TRANSPOSE_DEFAULT);
  printf(" -a, --attack <float>        Set the synth attack parameter      [%g]\n", ATTACK_DEFAULT);
  printf(" -d, --decay <float>         Set the synth decay parameter       [%g]\n", DECAY_DEFAULT);
  printf(" -s, --sustain <float>       Set the synth sustain parameter     [%g]\n", SUSTAIN_DEFAULT);
  printf(" -r, --release <float>       Set the synth release parameter     [%g]\n", RELEASE_DEFAULT);
  printf(" -g, --gain <float>          Set the synth gain                  [%g]\n", GAIN_DEFAULT);
  printf("\n");
}

int main (int argc, char ** argv)
{
  int opt;
  const char * options = "hm:n:u:t:a:d:s:r:g:pe:";
  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "pid", 0, NULL, 'p' },
    { "name", 1, NULL, 'e' },
    { "basic-name", 0, NULL, 'b' },
    { "modulation", 1, NULL, 'm' },
    { "harmonic", 1, NULL, 'n' },
    { "subharmonic", 1, NULL, 'u' },
    { "transpose", 1, NULL, 'y' },
    { "attack", 1, NULL, 'a' },
    { "decay", 1, NULL, 'd' },
    { "sustain", 1, NULL, 's' },
    { "release", 1, NULL, 'r' },
    { "gain", 1, NULL, 'g' },
    { 0, 0, 0, 0 }
  };
  lash_event_t * event;
  lash_args_t * lash_args;
#ifdef HAVE_GTK2
  pthread_t interface_thread;
#else
  pthread_t lash_thread;
#endif
  int flags = 0;

  modulation = MODULATION_DEFAULT;
  harmonic = HARMONIC_DEFAULT;
  subharmonic = SUBHARMONIC_DEFAULT;
  transpose = TRANSPOSE_DEFAULT;
  attack = ATTACK_DEFAULT;
  decay = DECAY_DEFAULT;  
  sustain = SUSTAIN_DEFAULT;
  release = RELEASE_DEFAULT;
  gain = GAIN_DEFAULT;
  
  lash_args = lash_extract_args (&argc, &argv);
  
#ifdef HAVE_GTK2
  gtk_init (&argc, &argv);
#endif

  sprintf (alsa_client_name, "%s (%d)", CLIENT_NAME_BASE, getpid ());
  sprintf (jack_client_name, "%s_%d", JACK_CLIENT_NAME_BASE, getpid ());

  while ((opt = getopt_long (argc, argv, options, long_options, NULL)) != -1)
    {
      switch (opt)
      {
      case 'm':
        modulation = atof (optarg);
        break;
      case 'n':
        harmonic = atoi (optarg);
        break;
      case 'u':
        subharmonic = atoi (optarg);
        break;
      case 't':
        transpose = atoi (optarg);
        break;
      case 'a':
        attack = atof (optarg);
        break;
      case 'd':
        decay = atof (optarg);
        break;
      case 's':
        sustain = atof (optarg);
        break;
      case 'r':
        release = atof (optarg);
        break;
      case 'g':
        gain = atof (optarg);
        break;
      case 'p':
      {
        pid_t pid;
        pid = getpid ();
        sprintf (alsa_client_name, "%s (%d)", CLIENT_NAME_BASE, pid);
        sprintf (jack_client_name, "%s_%d", JACK_CLIENT_NAME_BASE, pid);
        break;
      }
      case 'e':
        sprintf (alsa_client_name, "%s (%s)", CLIENT_NAME_BASE, optarg);
        sprintf (jack_client_name, "%s_%s", JACK_CLIENT_NAME_BASE, optarg);
        break;
      case 'b':
        sprintf (alsa_client_name, "%s", CLIENT_NAME_BASE);
        sprintf (jack_client_name, "%s", JACK_CLIENT_NAME_BASE);
        break;
      case 'h':
        print_help ();
        exit (0);
        break;
      case ':':
      case '?':
        print_help ();
        exit (1);
        break;
      }
    }


  flags = LASH_Config_Data_Set;
#ifndef HAVE_GTK2
  flags |= LASH_Terminal;
#endif

  lash_client = lash_init (lash_args, "LASH Synth", flags, LASH_PROTOCOL (2,0));
  
  if (!lash_client)
    {
      fprintf (stderr, "%s: could not initialise lash\n", __FUNCTION__);
      exit (1);
    }
  
  if (lash_enabled (lash_client))
    {
      event = lash_event_new_with_type (LASH_Client_Name);
      lash_event_set_string (event, alsa_client_name);
      lash_send_event (lash_client, event);
    }

#ifdef HAVE_GTK2
  pthread_create (&interface_thread, NULL, interface_main, NULL);
#else
  pthread_create (&lash_thread, NULL, lash_thread_main, NULL);
#endif

  synth_main (NULL);

  return 0;
}

