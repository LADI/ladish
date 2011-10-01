/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the libasound LD_PRELOAD-ed alsapid library
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "alsapid.h"

#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <linux/limits.h>
#include <stdio.h>

#define API_VERSION "ALSA_0.9"

// gcc -g -shared -Wl,--version-script=asound.versions -o libasound.so -fPIC -ldl -Wall -Werror lib.c helper.c
// LD_LIBRARY_PATH=alsapid LD_PRELOAD=libasound.so seq24

static void create_symlink(int alsa_client_id)
{
  char src[PATH_MAX];
  char dst[PATH_MAX];

  alsapid_compose_src_link(alsa_client_id, src);
  alsapid_compose_dst_link(dst);

  //printf("'%s' -> '%s'\n", src, dst);
  if (unlink(src) != 0 && errno != ENOENT)
  {
    fprintf(stderr, "unlink(\"%s\") failed with %d (%s)", src, errno, strerror(errno));
  }
  if (symlink(dst, src) != 0)
  {
    fprintf(stderr, "symlink(\"%s\", \"%s\") failed with %d (%s)", dst, src, errno, strerror(errno));
  }
}

static void destroy_symlink(int alsa_client_id)
{
  char src[PATH_MAX];

  alsapid_compose_src_link(alsa_client_id, src);

  //printf("'%s'\n", src);
  if (unlink(src) != 0 && errno != ENOENT)
  {
    fprintf(stderr, "unlink(\"%s\") failed with %d (%s)", src, errno, strerror(errno));
  }
}

//static int (* real_snd_seq_open)(snd_seq_t ** handle, const char * name, int streams, int mode);
static int (* real_snd_seq_set_client_name)(snd_seq_t * seq, const char * name);
static int (* real_snd_seq_close)(snd_seq_t * handle);
//static int (* real_snd_seq_create_port)(snd_seq_t * handle, snd_seq_port_info_t * info);
//static int (* real_snd_seq_create_simple_port)(snd_seq_t * seq, const char * name, unsigned int caps, unsigned int type);

#define CHECK_FUNC(func)                                                \
  if (real_ ## func == NULL)                                            \
  {                                                                     \
    real_ ## func = dlvsym(RTLD_NEXT, #func, API_VERSION);              \
    if (real_ ## func == NULL)                                          \
    {                                                                   \
      fprintf(stderr, "dlvsym(\""#func"\", \""API_VERSION"\") failed. %s\n", dlerror()); \
      return -1;                                                        \
    }                                                                   \
  }

#if 0
LADISH_PUBLIC
int snd_seq_open(snd_seq_t ** handle, const char * name, int streams, int mode)
{
  int ret;

  //printf("Attempt to create alsa client '%s', %d streams and mode %d\n", name, streams, mode);

  CHECK_FUNC(snd_seq_open);

  //printf("real_snd_seq_open = %p\n", real_snd_seq_open);
  ret = real_snd_seq_open(handle, name, streams, mode);
  if (ret == 0)
  {
    //printf("alsa client %p (%d) created\n", *handle, snd_seq_client_id(*handle));
  }
  return ret;
}
#endif

LADISH_PUBLIC
int snd_seq_set_client_name(snd_seq_t * seq, const char * name)
{
  int ret;

  //printf("Attempt to set alsa client name to '%s' of %p\n", name, seq); getchar();

  CHECK_FUNC(snd_seq_set_client_name);

  ret = real_snd_seq_set_client_name(seq, name);

  //printf("name set?\n"); getchar();

  if (ret == 0)
  {
    //printf("ALSAPID: pid = %lld SETNAME %d '%s'\n", (long long)getpid(), snd_seq_client_id(seq), name);
    create_symlink(snd_seq_client_id(seq));
  }

  return ret;
}

LADISH_PUBLIC
int snd_seq_close(snd_seq_t * handle)
{
  CHECK_FUNC(snd_seq_close);

  //printf("ALSAPID: pid = %lld CLOSE %d\n", (long long)getpid(), snd_seq_client_id(handle));
  destroy_symlink(snd_seq_client_id(handle));

  return real_snd_seq_close(handle);
}

#if 0
LADISH_PUBLIC
int snd_seq_create_port(snd_seq_t * handle, snd_seq_port_info_t * info)
{
  int ret;

  CHECK_FUNC(snd_seq_create_port);

  ret = real_snd_seq_create_port(handle, info);

  //printf("port created?\n"); getchar();

  return ret;
}

LADISH_PUBLIC
int snd_seq_create_simple_port(snd_seq_t * seq, const char * name, unsigned int caps, unsigned int type)
{
  int ret;

  CHECK_FUNC(snd_seq_create_simple_port);

  ret = real_snd_seq_create_simple_port(seq, name, caps, type);

  //printf("simple port created?\n"); getchar();

  return ret;
}
#endif
