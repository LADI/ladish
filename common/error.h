/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains error type declarations
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

#ifndef ERROR_H__00CE2C9B_1A90_4C55_8038_952441B43093__INCLUDED
#define ERROR_H__00CE2C9B_1A90_4C55_8038_952441B43093__INCLUDED

#include <stdint.h>

typedef uint32_t lash_error_t;

#define lash_is_error(x) (x != 0)

#define lash_success                  ((uint32_t)0)
#define lash_error_general            ((uint32_t)1)
#define lash_error_project_not_found  ((uint32_t)2)

#endif /* #ifndef ERROR_H__00CE2C9B_1A90_4C55_8038_952441B43093__INCLUDED */
