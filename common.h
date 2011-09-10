/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains stuff that is needed almost everywhere in ladish
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

#ifndef COMMON_H__82C9504A_ACD2_435D_9743_781943473E6A__INCLUDED
#define COMMON_H__82C9504A_ACD2_435D_9743_781943473E6A__INCLUDED

#include "config.h"             /* configure stage result */

#include <stdbool.h>            /* C99 bool */
#include <stdint.h>             /* fixed bit size ints */
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef __cplusplus
#include "common/klist.h"
#endif

#include "log.h"    /* log macros */
#include "assert.h" /* assert macros */

#define ladish_max(a, b) ((a) > (b) ? (a) : (b))
#define ladish_min(a, b) ((a) < (b) ? (a) : (b))

extern const char * g_app_L0_constant;
extern const char * g_app_L1_constant;
extern const char * g_app_L2_lash_constant;
extern const char * g_app_L2_js_constant;
#define LADISH_APP_LEVEL_0             g_app_L0_constant
#define LADISH_APP_LEVEL_1             g_app_L1_constant
#define LADISH_APP_LEVEL_LASH          g_app_L2_lash_constant
#define LADISH_APP_LEVEL_JACKSESSION   g_app_L2_js_constant
const char * ladish_map_app_level_constant(const char * level);

#define LADISH_PUBLIC __attribute__ ((visibility ("default")))

#endif /* #ifndef COMMON_H__82C9504A_ACD2_435D_9743_781943473E6A__INCLUDED */
