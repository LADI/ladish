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

#include <netinet/in.h>
#include <stdio.h>

#include <lash/config.h>
#include <lash/xmalloc.h>
#include <lash/internal.h>

void
lash_config_init (lash_config_t * config)
{
  config->key = NULL;
  config->value = NULL;
  config->value_size = 0;
}

lash_config_t *
lash_config_new ()
{
  lash_config_t * config;
  
  config = lash_malloc (sizeof (lash_config_t));
  lash_config_init (config);
  
  return config;
}

lash_config_t *
lash_config_dup (const lash_config_t * config)
{
  lash_config_t * config_dup;
  
  config_dup = lash_config_new ();
  lash_config_set_key (config_dup, lash_config_get_key (config));
  lash_config_set_value (config_dup, lash_config_get_value (config),
                                    lash_config_get_value_size (config));
  
  return config_dup;
}

lash_config_t *
lash_config_new_with_key (const char * key)
{
  lash_config_t * config;
  config = lash_config_new ();
  lash_config_set_key (config, key);
  return config;
}

void
lash_config_free (lash_config_t * config)
{
  if (config->key)
    {
      free (config->key);
      config->key = NULL;
    }
    
  if (config->value)
    {
      free (config->value);
      config->value = NULL;
    }
  
  config->value_size = 0;
}

void
lash_config_destroy (lash_config_t * config)
{
  lash_config_free (config);
  free (config);
}

const char *
lash_config_get_key (const lash_config_t * config)
{
  return config->key;
}

const void *
lash_config_get_value (const lash_config_t * config)
{
  return config->value;
}

size_t
lash_config_get_value_size (const lash_config_t * config)
{
  return config->value_size;
}

uint32_t
lash_config_get_value_int (const lash_config_t * config)
{
  const uint32_t * value;
  
  value = (const uint32_t *) lash_config_get_value (config);
  
  return ntohl(*value);
}

float
lash_config_get_value_float (const lash_config_t * config)
{
  const float * value;
  
  value = (const float *) lash_config_get_value (config);
  
  return *value;
}

double
lash_config_get_value_double (const lash_config_t * config)
{
  const double * value;
  
  value = (const double *) lash_config_get_value (config);
  
  return *value;
}

const char *
lash_config_get_value_string (const lash_config_t * config)
{
  return (const char *) lash_config_get_value (config);
}

void
lash_config_set_key (lash_config_t * config, const char * key)
{
  /* free the existing key */
  if (config->key)
    free (config->key);
  
  /* copy the new one */
  config->key = lash_strdup (key);
}

void
lash_config_set_value (lash_config_t * config, const void * value, size_t value_size)
{
  /* check whether the value size will survive the network */
  if (ntohl (htonl (value_size)) != value_size)
    {
      fprintf (stderr, "%s: value_size is too big and will not survive the network!\n",
               __FUNCTION__);
    }

  /* free the existing value */
  if (config->value)
    free (config->value);
  
  if (value)
    {
      config->value = lash_malloc (value_size);
      memcpy (config->value, value, value_size);
      
      config->value_size = value_size;
    }
  else
    {
      config->value = NULL;
      config->value_size = (size_t) 0;
    }
}

void
lash_config_set_value_int (lash_config_t * config, uint32_t value)
{
  uint32_t nvalue;
  
  nvalue = htonl (value);
  
  lash_config_set_value (config, &nvalue, sizeof (uint32_t));
}

void
lash_config_set_value_float (lash_config_t * config, float value)
{
  lash_config_set_value (config, &value, sizeof (float));
}

void
lash_config_set_value_double (lash_config_t * config, double value)
{
  lash_config_set_value (config, &value, sizeof (value));
}

void
lash_config_set_value_string (lash_config_t * config, const char * value)
{
  lash_config_set_value (config, value, strlen (value) + 1);
}


/* EOF */
