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

#ifndef APPDB_H__4839D031_68EF_43F5_BDE2_2317C6B956A9__INCLUDED
#define APPDB_H__4839D031_68EF_43F5_BDE2_2317C6B956A9__INCLUDED

#include "../common/klist.h"

/* all strings except name can be not present (NULL) */
/* all strings are utf-8 */
struct lash_appdb_entry
{
	struct list_head siblings;
	char * name;		/* Specific name of the application, for example "Ingen" */
	char * generic_name;	/* Generic name of the application, for example "Audio Editor" */
	char * comment;		/* Tooltip for the entry, for example "Record and edit audio files" */
	char * icon;		/* Icon */
	char * exec;		/* Program to execute, possibly with arguments. */
	char * path;		/* The working directory to run the program in. */
	bool terminal;		/* Wheter to run application in terminal */
};

/* parses .desktop entries in suitable XDG directories and returns list of lash_appdb_entry structs in appdb parameter */
/* returns success status */
bool
lash_appdb_load(
	struct list_head * appdb);

/* free list of lash_appdb_entry structs, as returned by lash_appdb_load() */
void
lash_appdb_free(
	struct list_head * appdb);

#endif /* #ifndef APPDB_H__4839D031_68EF_43F5_BDE2_2317C6B956A9__INCLUDED */
