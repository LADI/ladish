/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

/* For strdup() */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE 1
#endif

#include <stdint.h>
#include <string.h>

#include "common/safety.h"
#include "common/debug.h"

void
_lash_free(void **ptr)
{
	if (ptr && *ptr) {
		free(*ptr);
		*ptr = NULL;
	}
}

void
lash_strset(char       **property,
            const char  *value)
{
	if (property) {
		if (*property)
			free(*property);

		if (value)
			*property = lash_strdup(value);
		else
			*property = NULL;
	}
}

#ifndef LASH_DEBUG

void *
lash_malloc(size_t nmemb,
            size_t size)
{
	void *ptr = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr = malloc(nmemb * size))) {
				return ptr;
			} else {
				lash_error("malloc returned NULL");
			}
		} else {
			lash_error("Arguments would overflow");
		}
	} else {
		lash_error("Can't allocate zero bytes");
	}

	abort();
}

void *
lash_calloc(size_t nmemb,
            size_t size)
{
	void *ptr = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr = calloc(nmemb, size))) {
				return ptr;
			} else {
				lash_error("calloc returned NULL");
			}
		} else {
			lash_error("Arguments would overflow");
		}
	} else {
		lash_error("Can't allocate zero bytes");
	}

	abort();
}

void *
lash_realloc(void   *ptr,
             size_t  nmemb,
             size_t  size)
{
	void *ptr2 = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr2 = realloc(ptr, nmemb * size))) {
				return ptr2;
			} else {
				lash_error("realloc returned NULL");
			}
		} else {
			lash_error("Arguments would overflow");
		}
	} else {
		lash_error("Can't allocate zero bytes");
	}

	abort();
}

char *
lash_strdup(const char *s)
{
	char *ptr = (s) ? strdup(s) : strdup("");

	if (ptr)
		return ptr;

	lash_error("strdup returned NULL");

	abort();
}

char *
lash_catdup(const char *s1,
            const char *s2)
{
	char *str;

	if (s1 && s2) {
		size_t s1_len, s2_len;

		s1_len = strlen(s1);
		s2_len = strlen(s2);
		str = lash_malloc(1, s1_len + s2_len + 1);

		memcpy(str, s1, s1_len);
		memcpy(str + s1_len, s2, s2_len);
		str[s1_len + s2_len] = '\0';
	} else if (!(str = strdup(""))) {
		lash_error("strdup returned NULL");
		abort();
	}

	return str;
}

#else /* LASH_DEBUG */

void *
lash_malloc_dbg(size_t      nmemb,
                size_t      size,
                const char *file,
                int         line,
                const char *function,
                const char *arg1,
                const char *arg2)
{
	void *ptr = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr = malloc(nmemb * size))) {
				return ptr;
			} else {
				lash_error_plain("%s:%d:%s: "
				                 "lash_malloc(%s,%s) failed: "
				                 "malloc returned NULL",
				                 file, line, function,
				                 arg1, arg2);
			}
		} else {
			lash_error_plain("%s:%d:%s: "
			                 "lash_malloc(%s,%s) failed: "
			                 "Arguments would overflow",
			                 file, line, function, arg1, arg2);
		}
	} else {
		lash_error_plain("%s:%d:%s: "
		                 "lash_malloc(%s,%s) failed: "
		                 "Can't allocate zero bytes",
		                 file, line, function, arg1, arg2);
	}

	abort();
}

void *
lash_calloc_dbg(size_t      nmemb,
                size_t      size,
                const char *file,
                int         line,
                const char *function,
                const char *arg1,
                const char *arg2)
{
	void *ptr = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr = calloc(nmemb, size))) {
				return ptr;
			} else {
				lash_error_plain("%s:%d:%s: "
				                 "lash_calloc(%s,%s) failed: "
				                 "calloc returned NULL",
				                 file, line, function,
				                 arg1, arg2);
			}
		} else {
			lash_error_plain("%s:%d:%s: "
			                 "lash_calloc(%s,%s) failed: "
			                 "Arguments would overflow",
			                 file, line, function, arg1, arg2);
		}
	} else {
		lash_error_plain("%s:%d:%s: "
		                 "lash_calloc(%s,%s) failed: "
		                 "Can't allocate zero bytes",
		                 file, line, function, arg1, arg2);
	}

	abort();
}

void *
lash_realloc_dbg(void       *ptr,
                 size_t      nmemb,
                 size_t      size,
                 const char *file,
                 int         line,
                 const char *function,
                 const char *arg1,
                 const char *arg2,
                 const char *arg3)
{
	void *ptr2 = NULL;

	if (nmemb && size) {
		if (size <= (SIZE_MAX / nmemb)) {
			if ((ptr2 = realloc(ptr, nmemb * size))) {
				return ptr2;
			} else {
				lash_error_plain("%s:%d:%s: "
				                 "lash_realloc(%s,%s,%s) failed: "
				                 "calloc returned NULL",
				                 file, line, function,
				                 arg1, arg2, arg3);
			}
		} else {
			lash_error_plain("%s:%d:%s: "
			                 "lash_realloc(%s,%s,%s) failed: "
			                 "Arguments would overflow",
			                 file, line, function, arg1, arg2, arg3);
		}
	} else {
		lash_error_plain("%s:%d:%s: "
		                 "lash_realloc(%s,%s,%s) failed: "
		                 "Can't allocate zero bytes",
		                 file, line, function, arg1, arg2, arg3);
	}

	abort();
}

char *
lash_strdup_dbg(const char *s,
                const char *file,
                int         line,
                const char *function,
                const char *arg1)
{
	char *ptr;

	if (s) {
		ptr = strdup(s);
	} else {
		lash_error_plain("%s:%d:%s: lash_strdup(%s) failed: "
		                 "Argument is NULL",
		                 file, line, function, arg1);
		ptr = strdup("");
	}

	if (ptr)
		return ptr;

	lash_error_plain("%s:%d:%s: lash_strdup(%s) failed: "
	                 "strdup returned NULL",
	                 file, line, function, arg1);

	abort();
}

char *
lash_catdup_dbg(const char *s1,
                const char *s2,
                const char *file,
                int         line,
                const char *function,
                const char *arg1,
                const char *arg2)
{
	char *str;

	if (s1 && s2) {
		size_t s1_len, s2_len;

		s1_len = strlen(s1);
		s2_len = strlen(s2);
		str = lash_malloc(1, s1_len + s2_len + 1);

		memcpy(str, s1, s1_len);
		memcpy(str + s1_len, s2, s2_len);
		str[s1_len + s2_len] = '\0';
	} else {
		lash_error_plain("%s:%d:%s: lash_catdup(%s, %s) failed: "
		                 "NULL argument(s)",
		                 file, line, function, arg1, arg2);

		if (!(str = strdup(""))) {
			lash_error_plain("%s:%d:%s: lash_catdup(%s, %s) failed: "
			                 "strdup returned NULL",
			                 file, line, function, arg1, arg2);
			abort();
		}
	}

	return str;
}

#endif /* LASH_DEBUG */

/* EOF */
