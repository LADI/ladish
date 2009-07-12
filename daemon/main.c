/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "version.h"

#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <jack/jack.h>
#include <libxml/tree.h>

#include "../common/debug.h"

#include "server.h"
#include "loader.h"
//#include "version.h"

#ifdef LASH_DEBUG
#  include <mcheck.h>
#endif

#include "sigsegv.h"
#include "proctitle.h"

void
term_handler(int signum)
{
  lash_info("Caught signal %d (%s), terminating", signum, strsignal(signum));
  g_server->quit = true;
}

int
main(int    argc,
     char **argv,
     char **envp)
{
  int opt;
  const char *options = "hd:";
  struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"default-dir", 1, NULL, 'd'},
    {0, 0, 0, 0}
  };
  char *default_dir = NULL;
  sig_t sigh;
  struct stat st;
  char timestamp_str[26];

  st.st_mtime = 0;
  stat(argv[0], &st);
  ctime_r(&st.st_mtime, timestamp_str);
  timestamp_str[24] = 0;

  lash_init_setproctitle(argc, argv, envp);

  dbus_threads_init_default();

#ifdef LASH_DEBUG
  mtrace();
#endif

  xmlSetCompressMode(0);

  while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {
    switch (opt) {
    case 'd':
      default_dir = optarg;
      break;
    default:
      exit(EXIT_FAILURE);
      break;
    }
  }

  if (!default_dir)
    default_dir = DEFAULT_PROJECT_DIR;

  lash_info("------------------");
  lash_info("LASH activated. Version %s (%s) built on %s", PACKAGE_VERSION, GIT_VERSION, timestamp_str);

  lash_debug("Default dir: '%s'", default_dir);

  loader_init();

  if (!server_start(default_dir))
  {
    goto uninit_loader;
  }

  /* install the signal handlers */
  sigh = signal(SIGTERM, term_handler);
  if (sigh == SIG_IGN)
    signal(SIGTERM, SIG_IGN);

  sigh = signal(SIGINT, term_handler);
  if (sigh == SIG_IGN)
    signal(SIGINT, SIG_IGN);

  sigh = signal(SIGHUP, term_handler);
  if (sigh == SIG_IGN)
    signal(SIGHUP, SIG_IGN);

  signal(SIGPIPE, SIG_IGN);

  /* setup our SIGSEGV magic that prints nice stack in our logfile */ 
  setup_sigsegv();

  server_main();

  lash_debug("Finished, cleaning up");
  printf("Cleaning up\n");

  server_stop();

uninit_loader:
  loader_uninit();

  lash_debug("Cleaned up, exiting");
  printf("Finished\n");

  lash_info("LASH deactivated");
  lash_info("------------------");

  exit(EXIT_SUCCESS);
}
