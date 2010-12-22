/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains prototype of the catdup() function
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

#ifndef CATDUP_H__D42302F1_4D96_4EE4_AC09_E97ED5748277__INCLUDED
#define CATDUP_H__D42302F1_4D96_4EE4_AC09_E97ED5748277__INCLUDED

char * catdup(const char * s1, const char * s2);
char * catdup3(const char * s1, const char * s2, const char * s3);
char * catdup4(const char * s1, const char * s2, const char * s3, const char * s4);
char * catdupv(const char * s1, const char * s2, ...);
char * catdup_array(const char ** array, const char * delimiter);

#endif /* #ifndef CATDUP_H__D42302F1_4D96_4EE4_AC09_E97ED5748277__INCLUDED */
