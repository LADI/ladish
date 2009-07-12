/* -*- Mode: C -*- */
/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#ifndef PROCTITLE_H__028B89C9_1E78_4EE4_9E01_FDF77C856CF0__INCLUDED
#define PROCTITLE_H__028B89C9_1E78_4EE4_9E01_FDF77C856CF0__INCLUDED

#if defined(__linux__)

void
lash_init_setproctitle(
	int argc,
	char ** argv,
	char ** envp);

void
lash_setproctitile(
	const char * format,
	...);

#elif defined(__FreeBSD__)
# define lash_init_setproctitle(argc, argv, envp) ((void)0)
# define lash_setproctitle setproctitle
#else
# define lash_init_setproctitle(argc, argv, envp) ((void)0)
# define lash_setproctitle(format, ...) ((void)0)
#endif

#endif /* #ifndef PROCTITLE_H__028B89C9_1E78_4EE4_9E01_FDF77C856CF0__INCLUDED */
