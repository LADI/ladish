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
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_DEBUG_H__
#define __LASH_DEBUG_H__

#include <stdio.h>

#include "config.h"

#ifdef LASH_DEBUG
#define LASH_PRINT_DEBUG(string) fprintf(stderr, "%s:%d:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, string);
#define LASH_DEBUGARGS(format, ...) fprintf(stderr, "%s:%d:%s: " format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); 
#else
#define LASH_PRINT_DEBUG(string)
#define LASH_DEBUGARGS(format, ...)
#endif

#endif /* __LASH_DEBUG_H__ */
