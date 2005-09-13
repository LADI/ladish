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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mcheck.h>
#include <limits.h>

#include <jack/jack.h>
#include <libxml/tree.h>
#define XML_COMPRESSION_LEVEL 5

#include <lash/lash.h>
#include <lash/loader.h>
#include <lash/debug.h>

#include "config.h"
#include "conn_mgr.h"
#include "server.h"

int        no_v6         = 0;
server_t * global_server = NULL;

void
term_handler (int signum)
{
  LASH_DEBUGARGS ("recieved signal %d (%s), terminating", signum, strsignal (signum));

  global_server->quit = 1;
}

/*void
loader_quit_handler (int signum)
{
  LASH_DEBUGARGS ("recieved signal %s; setting loader's quit flag", strsignal (signum));
  
  server_set_loader_quit (global_server, 1);
}*/

static void
print_help ()
{
  printf("lashd version %s\n", PACKAGE_VERSION);
  printf("Copyright (C) 2002 Robert Ham <rah@bash.sh>\n");
  printf("\n");
  printf("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
  printf("under the terms of the GNU General Public License, version 2 or later.  See\n");
  printf("the COPYING file that came with this software for details.\n");
  printf("\n");
  printf("Compiled with ALSA %s, JACK %s and libxml2 %s\n", ALSA_VERSION, JACK_VERSION, XML2_VERSION);
  printf("\n");
  printf(" -h, --help                  Display this help info\n");
  printf(" -d, --default-dir <dir>     Use <dir> within $HOME to store project directories\n");
  printf(" -n, --no-ipv6               Do not use IPv6\n");
  printf("\n");
}
                            

int
main (int argc, char ** argv)
{
  int opt;
  const char * options = "hD:d:n";
  struct option long_options[] = {
    { "help",        0, NULL, 'h' },
    { "tmpdir",      1, NULL, 'D' },
    { "default-dir", 1, NULL, 'd' },
    { "no-ipv6",     0, NULL, 'n' },
    { 0, 0, 0, 0 }
  };
  char * default_dir = NULL;
  sighandler_t sigh;

  
#ifdef LASH_DEBUG
  mtrace ();
#endif 

  xmlSetCompressMode (XML_COMPRESSION_LEVEL);

  while ((opt = getopt_long (argc, argv, options, long_options, NULL)) != -1)
    {
      switch (opt)
      {
      case 'h':
        print_help ();
        exit (0);
        break;
      case 'd':
        default_dir = lash_strdup (optarg);
        break;
      case 'n':
	no_v6 = 1;
        break;
        
      case ':':
        print_help ();
        exit (1);
        break;
      case '?':
        print_help ();
        exit (1);
        break;
    }
  }

  if (!default_dir)
    default_dir = DEFAULT_PROJECT_DIR;

  LASH_DEBUGARGS ("default dir: '%s'", default_dir);
  
  global_server = server_new (default_dir);  

  /* install the signal handlers */
  sigh = signal (SIGTERM, term_handler);
  if (sigh == SIG_IGN)
    signal (SIGTERM, SIG_IGN);
    
  sigh = signal (SIGINT, term_handler);
  if (sigh == SIG_IGN)
    signal (SIGINT, SIG_IGN);
    
  sigh = signal (SIGHUP, term_handler);
  if (sigh == SIG_IGN)
    signal (SIGHUP, SIG_IGN);

  signal (SIGCHLD, term_handler);
  signal (SIGPIPE, SIG_IGN);



  server_main (global_server);
  
  printf ("Cleaning up\n");
    
  LASH_PRINT_DEBUG ("finished, cleaning up");
  
  server_destroy (global_server);
  
  LASH_PRINT_DEBUG ("cleaned up, exiting");
  printf ("Finished\n");
  exit (0);
}

