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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include "lash/lash.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lash_control.h"

static void
print_help()
{
	printf("LASH Control version %s\n", PACKAGE_VERSION);
	printf("Copyright (C) 2002 Robert Ham <rah@bash.sh>\n");
	printf("\n");
	printf
		("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
	printf
		("under the terms of the GNU General Public License, version 2 or later.  See\n");
	printf("the COPYING file that came with this software for details.\n");
	printf("\n");
	printf(" -h, --help                  Display this help info\n");
	printf
		("     --lash-server <host>  Connect to server running on <host>\n");
	printf("     --lash-port <port>    Connect to server on port <port>\n");
	printf("\n");
}

int
main(int argc, char **argv)
{
	lash_control_t control;
	lash_args_t *lash_args;
	lash_client_t *lash_client;
	int opt;
	const char *options = "h";
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0}
	};

	lash_args = lash_extract_args(&argc, &argv);

	while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			exit(0);
			break;

		case ':':
			print_help();
			exit(1);
			break;
		case '?':
			print_help();
			exit(1);
			break;
		}
	}

	lash_client =
		lash_init(lash_args, "LASH Control",
				  LASH_Server_Interface | LASH_Terminal, LASH_PROTOCOL(2, 0));

	if (!lash_client) {
		fprintf(stderr, "%s: could not initialise lash\n", __FUNCTION__);
		exit(1);
	}

	lash_control_init(&control);
	control.client = lash_client;

	lash_control_main(&control);

	return 0;
}
