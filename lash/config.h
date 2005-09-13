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

#ifndef __LASH_CONFIG_H__
#define __LASH_CONFIG_H__

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#include <lash/lash.h>

#ifdef __cplusplus
extern "C" {
#endif

lash_config_t * lash_config_new          (void);
lash_config_t * lash_config_dup          (const lash_config_t * config);
lash_config_t * lash_config_new_with_key (const char * key);
void           lash_config_destroy      (lash_config_t * config);


const char * lash_config_get_key        (const lash_config_t * config);
const void * lash_config_get_value      (const lash_config_t * config);
size_t       lash_config_get_value_size (const lash_config_t * config);

void lash_config_set_key   (lash_config_t * config, const char * key);
void lash_config_set_value (lash_config_t * config, const void * value, size_t value_size);



/*
 * with these functions, no type checking is done; you can do
 * lash_config_get_value_int on a config that was set with
 * lash_config_set_value_float.
 *
 * the int values are converted to and from network byte order as
 * appropriate
 */
uint32_t     lash_config_get_value_int    (const lash_config_t * config);
float        lash_config_get_value_float  (const lash_config_t * config);
double       lash_config_get_value_double (const lash_config_t * config);
const char * lash_config_get_value_string (const lash_config_t * config);

void lash_config_set_value_int    (lash_config_t * config, uint32_t value);
void lash_config_set_value_float  (lash_config_t * config, float value);
void lash_config_set_value_double (lash_config_t * config, double value);
void lash_config_set_value_string (lash_config_t * config, const char * value);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_CONFIG_H__ */
