/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains prototypes of the directory helper functions
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

#ifndef DIRHELPERS_H__805193D2_2662_40FA_8814_AF8A4E08F4B0__INCLUDED
#define DIRHELPERS_H__805193D2_2662_40FA_8814_AF8A4E08F4B0__INCLUDED

bool ensure_dir_exist(const char * dirname, int mode);
bool ensure_dir_exist_varg(int mode, ...);

#endif /* #ifndef DIRHELPERS_H__805193D2_2662_40FA_8814_AF8A4E08F4B0__INCLUDED */
