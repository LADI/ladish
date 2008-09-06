/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <jack/jack.h>
#include <alsa/asoundlib.h>
#include <lash/lash.h>

#define info(fmt, args...) printf(fmt "\n", ## args)
#define error(fmt, args...) fprintf(stderr, "%s: " fmt "\n", __FUNCTION__, ## args)

/* Callback function prototypes */
static bool save_cb(lash_config_handle_t *handle, void *user_data);
static bool load_cb(lash_config_handle_t *handle, void *user_data);
static bool quit_cb(void *user_data);

int
main(int    argc,
     char **argv)
{
	lash_client_t *client;
	jack_client_t *jack_client;
	snd_seq_t *aseq;
	snd_seq_client_info_t *aseq_client_info;
	char client_name[64];
	bool done = false;

	sprintf(client_name, "lsec_%d", getpid());
	info("Client name: '%s'", client_name);

	info("Attempting to initialise LASH");

	client = lash_client_open("LASH Simple Client", LASH_Config_Data_Set,
	                          argc, argv);
	if (!client) {
		error("Could not initialise LASH");
		return EXIT_FAILURE;
	}

	info("Connected to LASH with client name '%s'\n"
	     "Associated with project '%s'",
	     lash_get_client_name(client),
	     lash_get_project_name(client));

	/* JACK */
	info("Connecting to JACK server");
	jack_client = jack_client_open(client_name, JackNullOption, NULL);
	if (!jack_client) {
		error("Could not connect to JACK server");
		return EXIT_FAILURE;
	}
	info("Connected to JACK with client name '%s'", client_name);

	/* ALSA */
	info("Opening ALSA sequencer");
	if (snd_seq_open(&aseq, "default", SND_SEQ_OPEN_DUPLEX, 0) != 0) {
		error("Could not open ALSA sequencer");
		return EXIT_FAILURE;
	}
	snd_seq_client_info_alloca(&aseq_client_info);
	snd_seq_get_client_info(aseq, aseq_client_info);
	snd_seq_client_info_set_name(aseq_client_info, client_name);
	snd_seq_set_client_info(aseq, aseq_client_info);
	info("Opened ALSA sequencer with ID %d, name '%s'",
	     snd_seq_client_id(aseq), client_name);

	lash_jack_client_name(client, client_name);
	lash_alsa_client_id(client, (unsigned char) snd_seq_client_id(aseq));

	lash_set_save_data_set_callback(client, save_cb, NULL);
	lash_set_load_data_set_callback(client, load_cb, NULL);
	lash_set_quit_callback(client, quit_cb, &done);

	while (!done) {
		lash_wait(client);
		lash_dispatch(client);
	}

	info("Bye!");

	return EXIT_SUCCESS;
}

static bool
save_cb(lash_config_handle_t *handle,
        void                 *user_data)
{
	info("Received SaveDataSet message");

	const char *name = "lash_simple_client";
	double version = 1.0;
	uint64_t revision = 1;
	const char *raw_data = "some raw data";

	lash_config_write(handle, "name", &name, LASH_TYPE_STRING);
	lash_config_write(handle, "version", &version, LASH_TYPE_DOUBLE);
	lash_config_write(handle, "revision", &revision, LASH_TYPE_INTEGER);
	/* Write a string as raw data without the terminating NUL */
	lash_config_write_raw(handle, "raw_data", raw_data, strlen(raw_data));

	return true;
}

static bool
load_cb(lash_config_handle_t *handle,
        void                 *user_data)
{
	info("Received LoadDataSet message");

	const char *key;
	int ret, type;

	union {
		double      d;
		uint32_t    u;
		const char *s;
	} value;

	while ((ret = lash_config_read(handle, &key, &value, &type))) {
		if (ret == -1) {
			error("Failed to read data set");
			return false;
		}

		if (type == LASH_TYPE_DOUBLE)
			info("  \"%s\" : %f", key, value.d);
		else if (type == LASH_TYPE_INTEGER)
			info("  \"%s\" : %u", key, value.u);
		else if (type == LASH_TYPE_STRING)
			info("  \"%s\" : \"%s\"", key, value.s);
		else if (type == LASH_TYPE_RAW)
			info("  \"%s\" : <raw data>", key);
		else
			error("Unknown config type");
	}

	return true;
}

static bool
quit_cb(void *user_data)
{
	info("Received Quit message");
	*((bool *) user_data) = true;
	return true;
}
