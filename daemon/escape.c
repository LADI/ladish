/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the escape helper functions
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

#include "escape.h"

static char hex_digits[] = "0123456789ABCDEF";

#define HEX_TO_INT(hexchar) ((hexchar) <= '9' ? hexchar - '0' : 10 + (hexchar - 'A'))

void escape(const char ** src_ptr, char ** dst_ptr)
{
  const char * src;
  char * dst;

  src = *src_ptr;
  dst = *dst_ptr;

  while (*src != 0)
  {
    switch (*src)
    {
    case '/':               /* used as separator for address components */
    case '<':               /* invalid attribute value char (XML spec) */
    case '&':               /* invalid attribute value char (XML spec) */
    case '"':               /* we store attribute values in double quotes - invalid attribute value char (XML spec) */
    case '\'':
    case '>':
    case '%':
      dst[0] = '%';
      dst[1] = hex_digits[*src >> 4];
      dst[2] = hex_digits[*src & 0x0F];
      dst += 3;
      src++;
      break;
    default:
      *dst++ = *src++;
    }
  }

  *src_ptr = src;
  *dst_ptr = dst;
}

void escape_simple(const char * src_ptr, char * dst_ptr)
{
  escape(&src_ptr, &dst_ptr);
}

size_t unescape(const char * src, size_t src_len, char * dst)
{
  size_t dst_len;

  dst_len = 0;

  while (src_len)
  {
    if (src_len >= 3 &&
        src[0] == '%' &&
        ((src[1] >= '0' && src[1] <= '9') ||
         (src[1] >= 'A' && src[1] <= 'F')) &&
        ((src[2] >= '0' && src[2] <= '9') ||
         (src[2] >= 'A' && src[2] <= 'F')))
    {
      *dst = (HEX_TO_INT(src[1]) << 4) | HEX_TO_INT(src[2]);
      //lash_info("unescaping %c%c%c to '%c'", src[0], src[1], src[2], *dst);
      src_len -= 3;
      src += 3;
    }
    else
    {
      *dst = *src;
      src_len--;
      src++;
    }
    dst++;
    dst_len++;
  }

  return dst_len;
}
