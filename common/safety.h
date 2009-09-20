/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to "safe" memory and string helpers (obsolete)
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

/** @file safety.h
 *   This module contains wrappers around system memory handling functions
 *   for reliable error handling and increased debugging verbosity.
 *
 *   Some of the provided methods can be either macros or function definitions
 *   depending on whether or not LADISH_DEBUG is enabled. The programmer needn't
 *   worry, however, as the syntax is the same for both debug and normal builds.
 */

#ifndef __LASH_SAFETY_H__
#define __LASH_SAFETY_H__

#include <stdlib.h>

#include "config.h"

/**
 * This macro is a convenience wrapper around \ref _lash_free.
 * It does nothing more than cast the provided argument to void**
 * in order to eliminate invalid pointer type warnings.
 */
#define lash_free(ptr) \
  _lash_free((void **) ptr)

/**
 * Free the memory pointed to by *ptr. If the pointer was valid,
 * set it to NULL after freeing it.
 *
 * @param ptr A pointer to a pointer to the memory to be be freed.
 */
void
_lash_free(void **ptr);

/**
 * If value is non-NULL, set *property to point to a new string
 * which is a copy of value. If value is NULL, set *property
 * to NULL. If *property is already set, it will be freed before setting
 * it to value.
 *
 * @param property A pointer to the char* to set.
 * @param value The value to set property to.
 */
void
lash_strset(char       **property,
            const char  *value);

#ifndef LADISH_DEBUG

/**
 * This is a wrapper around malloc which checks whether nmemb * size
 * would overflow, and whether malloc returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size in bytes of one element.
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_malloc(size_t nmemb,
            size_t size);

/**
 * This is a wrapper around calloc which checks whether nmemb * size
 * would overflow, and whether calloc) returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of one element (in bytes).
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_calloc(size_t nmemb,
            size_t size);

/**
 * This is a wrapper around realloc which checks whether nmemb * size
 * would overflow, and whether realloc returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param ptr A pointer to the memory block whose size to change.
 * @param nmemb The number of elements to (re)allocate.
 * @param size The size of one element (in bytes).
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_realloc(void   *ptr,
             size_t  nmemb,
             size_t  size);

/**
 * This is a wrapper around strdup which checks for a NULL return value.
 * It prints an error using \ref log_error if strdup fails.
 * 
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param s The string to duplicate.
 *
 * @return A pointer to the duplicated string.
 */
char *
lash_strdup(const char *s);

/** TODO: Document this */
#else /* LADISH_DEBUG */

/**
 * This macro is a wrapper around \ref lash_malloc_dbg.
 * 
 * @param nmemb The number of elements to allocate.
 * @param size The size in bytes of one element.
 *
 * @return A pointer to the allocated memory.
 */
# define lash_malloc(nmemb, size) \
  lash_malloc_dbg(nmemb, size, __FILE__, __LINE__, __func__, # nmemb, # size)

/**
 * This macro is a wrapper around \ref lash_calloc_dbg.
 * 
 * @param nmemb The number of elements to allocate.
 * @param size The size in bytes of one element.
 *
 * @return A pointer to the allocated memory.
 */
# define lash_calloc(nmemb, size) \
  lash_calloc_dbg(nmemb, size, __FILE__, __LINE__, __func__, # nmemb, # size)

/**
 * This macro is a wrapper around \ref lash_realloc_dbg.
 * 
 * @param ptr A pointer to the memory block whose size to change.
 * @param nmemb The number of elements to allocate.
 * @param size The size in bytes of one element.
 *
 * @return A pointer to the allocated memory.
 */
# define lash_realloc(ptr, nmemb, size) \
  lash_realloc_dbg(ptr, nmemb, size, __FILE__, __LINE__, __func__, # ptr, # nmemb, # size)

/**
 * This macro is a wrapper around \ref lash_strdup_dbg.
 * 
 * @param s The string to duplicate.
 *
 * @return A pointer to the duplicated string.
 */
# define lash_strdup(s) \
  lash_strdup_dbg(s, __FILE__, __LINE__, __func__, # s)

/**
 * This is a wrapper around malloc which checks whether nmemb * size
 * would overflow, and whether malloc returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * This function is the 'debugging edition' of the LASH malloc wrapper and
 * has extra parameters for printing more detailed debugging messages. It
 * should never be called directly, but instead by using the \ref lash_malloc
 * macro.
 *
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of one element (in bytes).
 * @param file The calling file name.
 * @param line The calling line number.
 * @param function The calling function name.
 * @param arg1 A string representation of the nmemb argument passed by the caller.
 * @param arg2 A string representation of the size argument passed by the caller.
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_malloc_dbg(size_t      nmemb,
                size_t      size,
                const char *file,
                int         line,
                const char *function,
                const char *arg1,
                const char *arg2);

/**
 * This is a wrapper around calloc which checks whether nmemb * size
 * would overflow, and whether calloc returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * This function is the 'debugging edition' of the LASH calloc wrapper and
 * has extra parameters for printing more detailed debugging messages. It
 * should never be called directly, but instead by using the \ref lash_calloc
 * macro.
 *
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of one element (in bytes).
 * @param file The calling file name.
 * @param line The calling line number.
 * @param function The calling function name.
 * @param arg1 A string representation of the nmemb argument passed by the caller.
 * @param arg2 A string representation of the size argument passed by the caller.
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_calloc_dbg(size_t      nmemb,
                size_t      size,
                const char *file,
                int         line,
                const char *function,
                const char *arg1,
                const char *arg2);

/**
 * This is a wrapper around realloc which checks whether nmemb * size
 * would overflow, and whether realloc returns NULL. It prints an error
 * using \ref log_error if anything fails.
 * 
 * This function is the 'debugging edition' of the LASH realloc wrapper and
 * has extra parameters for printing more detailed debugging messages. It
 * should never be called directly, but instead by using the \ref lash_realloc
 * macro.
 *
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of one element (in bytes).
 * @param file The calling file name.
 * @param line The calling line number.
 * @param function The calling function name.
 * @param arg1 A string representation of the ptr argument passed by the caller.
 * @param arg2 A string representation of the nmemb argument passed by the caller.
 * @param arg3 A string representation of the size argument passed by the caller.
 *
 * @return A pointer to the allocated memory.
 */
void *
lash_realloc_dbg(void       *ptr,
                 size_t      nmemb,
                 size_t      size,
                 const char *file,
                 int         line,
                 const char *function,
                 const char *arg1,
                 const char *arg2,
                 const char *arg3);

/**
 * This is a wrapper around strdup which checks for a NULL return value.
 * It prints an error using \ref log_error if strdup fails.
 * 
 * This function is the 'debugging edition' of the LASH strdup wrapper and
 * has extra parameters for printing more detailed debugging messages. It
 * should never be called directly, but instead by using the \ref lash_strdup
 * macro.
 *
 * TODO: We currently call abort on failure,
 *       this probably needs to be changed.
 *
 * @param s The string to duplicate.
 * @param file The calling file name.
 * @param line The calling line number.
 * @param function The calling function name.
 * @param arg1 A string representation of the argument passed by the caller.
 *
 * @return A pointer to the duplicated string.
 */
char *
lash_strdup_dbg(const char *s,
                const char *file,
                int         line,
                const char *function,
                const char *arg1);

#endif /* LADISH_DEBUG */

#endif /* __LASH_SAFETY_H__ */
