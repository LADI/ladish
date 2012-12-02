/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2012 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 * Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 **************************************************************************
 * This file contains log macros
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

#ifndef __LADISH_LOG_H__
#define __LADISH_LOG_H__

#define ANSI_BOLD_ON    "\033[1m"
#define ANSI_BOLD_OFF   "\033[22m"
#define ANSI_COLOR_RED  "\033[31m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_RESET      "\033[0m"

#include <stdio.h>

#include "config.h"

/* fallback for old gcc versions,
   http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#ifdef __cplusplus
extern "C"
#endif
void
ladish_log(
  unsigned int level,
  const char * file,
  unsigned int line,
  const char * func,
  const char * format,
  ...)
#if defined (__GNUC__)
  __attribute__((format(printf, 5, 6)))
#endif
  ;

#define LADISH_LOG_LEVEL_DEBUG        0
#define LADISH_LOG_LEVEL_INFO         1
#define LADISH_LOG_LEVEL_WARN         2
#define LADISH_LOG_LEVEL_ERROR        3
#define LADISH_LOG_LEVEL_ERROR_PLAIN  4

#define log_debug(fmt, args...)       ladish_log(LADISH_LOG_LEVEL_DEBUG,       __FILE__, __LINE__, __func__, fmt, ## args)
#define log_info(fmt, args...)        ladish_log(LADISH_LOG_LEVEL_INFO,        __FILE__, __LINE__, __func__, fmt, ## args)
#define log_warn(fmt, args...)        ladish_log(LADISH_LOG_LEVEL_WARN,        __FILE__, __LINE__, __func__, fmt, ## args)
#define log_error(fmt, args...)       ladish_log(LADISH_LOG_LEVEL_ERROR,       __FILE__, __LINE__, __func__, fmt, ## args)
#define log_error_plain(fmt, args...) ladish_log(LADISH_LOG_LEVEL_ERROR_PLAIN, __FILE__, __LINE__, __func__, fmt, ## args)

#endif /* __LADISH_LOG__ */
