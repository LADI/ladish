/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include <string.h>
#include <arpa/inet.h>
#include <rpc/xdr.h>

#include "common/debug.h"

#include "lash/config.h"

#include "lash_config.h"

bool
lash_config_write(lash_config_handle_t *handle,
                  const char           *key,
                  const void           *value,
                  int                   type)
{
	if (!handle || !key || !key[0] || !value) {
		lash_error("Invalid arguments");
		return false;
	}

	const void *value_ptr;
	char buf[8];

	if (handle->is_read) {
		lash_error("Cannot write config data during a LoadDataSet operation");
		return false;
	}

	if (type == LASH_TYPE_DOUBLE) {
		XDR x;
		xdrmem_create(&x, (char *) buf, 8, XDR_ENCODE);
		if (!xdr_double (&x, (double *) value)) {
			lash_error("Failed to encode floating point value");
			return false;
		}
		value_ptr = buf;
	} else if (type == LASH_TYPE_INTEGER) {
		*((uint32_t *) buf) = htonl(*((uint32_t *) value));
		value_ptr = buf;
	} else if (type == LASH_TYPE_STRING) {
		value_ptr = value;
	} else {
		lash_error("Invalid value type %i", type);
		return false;
	}

	lash_debug("Writing config \"%s\" of type '%c'", key, (char) type);

	if (!method_iter_append_dict_entry(handle->iter, type, key, value_ptr, 0)) {
		lash_error("Failed to append dict entry");
		return false;
	}

	return true;
}

bool
lash_config_write_raw(lash_config_handle_t *handle,
                      const char           *key,
                      const void           *buf,
                      int                   size)
{
	if (!handle || !key || !key[0] || !buf || size < 1) {
		lash_error("Invalid arguments");
		return false;
	}

	if (handle->is_read) {
		lash_error("Cannot write config data during a LoadDataSet operation");
		return false;
	}

	lash_debug("Writing raw config \"%s\"", key);

	if (!method_iter_append_dict_entry(handle->iter, LASH_TYPE_RAW, key, &buf, size)) {
		lash_error("Failed to append dict entry");
		return false;
	}

	return true;
}

int
lash_config_read(lash_config_handle_t  *handle,
                 const char           **key_ptr,
                 void                  *value_ptr,
                 int                   *type_ptr)
{
	if (!handle || !key_ptr || !value_ptr || !type_ptr) {
		lash_error("Invalid arguments");
		return -1;
	}

	int size;

	if (!handle->is_read) {
		lash_error("Cannot read config data during a SaveDataSet operation");
		return -1;
	}

	/* No data left in message */
	if (dbus_message_iter_get_arg_type(handle->iter) == DBUS_TYPE_INVALID)
		return 0;

	if (!method_iter_get_dict_entry(handle->iter, key_ptr, value_ptr,
	                                type_ptr, &size)) {
		lash_error("Failed to read config message");
		return -1;
	}

	dbus_message_iter_next(handle->iter);

	if (*type_ptr == LASH_TYPE_DOUBLE) {
		XDR x;
		double d;
		xdrmem_create(&x, (char *) value_ptr, 8, XDR_DECODE);
		if (!xdr_double (&x, &d)) {
			lash_error("Failed to decode floating point value");
			return -1;
		}
		*((double *) value_ptr) = d;
		size = sizeof(double);
	} else if (*type_ptr == LASH_TYPE_INTEGER) {
		uint32_t u = *((uint32_t *) value_ptr);
		*((uint32_t *) value_ptr) = ntohl(u);
		size = sizeof(uint32_t);
	} else if (*type_ptr == LASH_TYPE_STRING) {
		size = (int) strlen(*((const char **) value_ptr));
		if (size < 1) {
			lash_error("String length is 0");
			return -1;
		}
	} else if (*type_ptr != LASH_TYPE_RAW) {
		lash_error("Unknown value type %i", *type_ptr);
		return -1;
	}

	return size;
}


#ifdef LASH_OLD_API
# include "common/safety.h"

# include <stdlib.h>
# include <string.h>

# include "lash/types.h"

lash_config_t *
lash_config_new()
{
	return lash_calloc(1, sizeof(lash_config_t));
}

lash_config_t *
lash_config_dup(const lash_config_t *config)
{
	if (!config)
		return NULL;

	lash_config_t *config_dup;

	config_dup = lash_calloc(1, sizeof(lash_config_t));
	config_dup->key = lash_strdup(config->key);

	if (config->value && config->value_size > 0) {
		config_dup->value = lash_malloc(1, config->value_size);
		memcpy(config_dup->value, config->value, config->value_size);
		config_dup->value_size = config->value_size;
	}

	return config_dup;
}

lash_config_t *
lash_config_new_with_key(const char *key)
{
	lash_config_t *config;

	config = lash_calloc(1, sizeof(lash_config_t));
	if (key)
		config->key = lash_strdup(key);

	return config;
}

void
lash_config_free(lash_config_t *config)
{
	lash_free(&config->key);
	lash_free(&config->value);
	config->value_size = 0;
}

void
lash_config_destroy(lash_config_t *config)
{
	if (config) {
		lash_config_free(config);
		free(config);
	}
}

const char *
lash_config_get_key(const lash_config_t *config)
{
	return (const char *) (config ? config->key : NULL);
}

const void *
lash_config_get_value(const lash_config_t *config)
{
	return (const void *) (config ? config->value : NULL);
}

size_t
lash_config_get_value_size(const lash_config_t *config)
{
	return config ? config->value_size : 0;
}

uint32_t
lash_config_get_value_int(const lash_config_t *config)
{
	return config && config->value ? *((uint32_t *) config->value) : 0;
}

float
lash_config_get_value_float(const lash_config_t *config)
{
	return config && config->value ? (float) *((double *) config->value) : 0.0f;
}

double
lash_config_get_value_double(const lash_config_t *config)
{
	return config && config->value ? *((double *) config->value) : 0.0;
}

const char *
lash_config_get_value_string(const lash_config_t *config)
{
	return (const char *) (config ? config->value : NULL);
}

void
lash_config_set_key(lash_config_t *config,
                    const char    *key)
{
	if (config) {
		/* Free the existing key */
		if (config->key)
			free(config->key);

		/* Copy the new one */
		config->key = lash_strdup(key);
	}
}

void
lash_config_set_value(lash_config_t *config,
                      const void    *value,
                      size_t         value_size)
{
	if (config) {
		/* Free the existing value */
		if (config->value)
			free(config->value);

		if (!value || value_size < 1) {
			config->value = NULL;
			config->value_size = 0;
			return;
		}

		config->value = lash_malloc(1, value_size);
		memcpy(config->value, value, value_size);
		config->value_size = value_size;
		config->value_type = LASH_TYPE_RAW;
	}
}

void
lash_config_set_value_int(lash_config_t *config,
                          uint32_t       value)
{
	lash_config_set_value(config, &value, sizeof(uint32_t));
}

void
lash_config_set_value_float(lash_config_t *config,
                            float          value)
{
	double d = value;
	lash_config_set_value(config, &d, sizeof(double));
}

void
lash_config_set_value_double(lash_config_t *config,
                             double         value)
{
	lash_config_set_value(config, &value, sizeof(double));
}

void
lash_config_set_value_string(lash_config_t *config,
                             const char    *value)
{
	if (!value)
		lash_error("String is NULL");
	else
		lash_config_set_value(config, value, strlen(value) + 1);
}

#endif /* LASH_OLD_API */

/* EOF */
