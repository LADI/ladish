/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains storage for some the string constants.
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

#include "common.h"

const char * g_app_L0_constant = "0";
const char * g_app_L1_constant = "1";
const char * g_app_L2_lash_constant = "lash";
const char * g_app_L2_js_constant = "jacksession";

const char * ladish_map_app_level_constant(const char * level)
{
  const char * const levels[] =
  {
    g_app_L0_constant,
    g_app_L1_constant,
    g_app_L2_lash_constant,
    g_app_L2_js_constant,
    NULL
  };
  const char * const * ptr;

  for (ptr = levels; *ptr != NULL; ptr++)
  {
    if (strcmp(level, *ptr) == 0)
    {
      return *ptr;
    }
  }

  log_error("unknown app level constant '%s'", level);
  return NULL;
}
