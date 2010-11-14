/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains assert macros
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

#ifndef ASSERT_H__F236CB4F_D812_4636_958C_C82FD3781EC8__INCLUDED
#define ASSERT_H__F236CB4F_D812_4636_958C_C82FD3781EC8__INCLUDED

#include "log.h"

#include </usr/include/assert.h>

#define ASSERT(expr)                                                  \
  do                                                                  \
  {                                                                   \
    if (!(expr))                                                      \
    {                                                                 \
      log_error("ASSERT(" #expr ") failed. function %s in %s:%4u\n",  \
                __FUNCTION__,                                         \
                __FILE__,                                             \
                __LINE__);                                            \
      assert(false);                                                  \
    }                                                                 \
  }                                                                   \
  while(false)

#define ASSERT_NO_PASS                                                \
  do                                                                  \
  {                                                                   \
    log_error("Code execution taboo point. function %s in %s:%4u\n",  \
              __FUNCTION__,                                           \
              __FILE__,                                               \
              __LINE__);                                              \
    assert(false);                                                    \
  }                                                                   \
  while(false)

#endif /* #ifndef ASSERT_H__F236CB4F_D812_4636_958C_C82FD3781EC8__INCLUDED */
