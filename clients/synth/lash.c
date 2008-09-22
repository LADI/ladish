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

#define _GNU_SOURCE

#include "config.h"

#include <unistd.h>
#include <string.h>

#include "lash/lash.h"

#include "synth.h"

#ifdef HAVE_GTK2
#  include "interface.h"
#endif

void
save_data()
{
	lash_config_t *config;

	config = lash_config_new_with_key("modulation");
	lash_config_set_value_double(config, modulation);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("attack");
	lash_config_set_value_double(config, attack);
	lash_send_config(lash_client, config);
	//printf("attack saved as %f\n", attack);

	config = lash_config_new_with_key("decay");
	lash_config_set_value_double(config, decay);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("sustain");
	lash_config_set_value_double(config, sustain);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("release");
	lash_config_set_value_double(config, release);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("gain");
	lash_config_set_value_double(config, gain);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("harmonic");
	lash_config_set_value_int(config, harmonic);
	lash_send_config(lash_client, config);
	//printf("harmonic saved as %d\n", harmonic);

	config = lash_config_new_with_key("subharmonic");
	lash_config_set_value_int(config, subharmonic);
	lash_send_config(lash_client, config);

	config = lash_config_new_with_key("transpose");
	lash_config_set_value_int(config, transpose);
	lash_send_config(lash_client, config);

}

void
restore_data(lash_config_t * config)
{
	const char *key;

	key = lash_config_get_key(config);

	if (strcmp(key, "modulation") == 0) {
		modulation = lash_config_get_value_double(config);
		return;
	}

	if (strcmp(key, "attack") == 0) {
		attack = lash_config_get_value_double(config);
		//printf("attack restored to %f\n", attack);
		return;
	}

	if (strcmp(key, "decay") == 0) {
		decay = lash_config_get_value_double(config);
		return;
	}

	if (strcmp(key, "sustain") == 0) {
		sustain = lash_config_get_value_double(config);
		return;
	}

	if (strcmp(key, "release") == 0) {
		release = lash_config_get_value_double(config);
		return;
	}

	if (strcmp(key, "gain") == 0) {
		gain = lash_config_get_value_double(config);
		return;
	}

	if (strcmp(key, "harmonic") == 0) {
		harmonic = lash_config_get_value_int(config);
		//printf("harmonic restored to %d\n", harmonic);
		return;
	}

	if (strcmp(key, "subharmonic") == 0) {
		subharmonic = lash_config_get_value_int(config);
		return;
	}

	if (strcmp(key, "transpose") == 0) {
		transpose = lash_config_get_value_int(config);
		return;
	}

}

int
lash_main()
{
	lash_event_t *event;
	lash_config_t *config;

	while ((event = lash_get_event(lash_client))) {
		switch (lash_event_get_type(event)) {
		case LASH_Quit:
			printf("LASH ordered to quit\n");
			quit = 1;
			lash_event_destroy(event);
			break;
		case LASH_Restore_Data_Set:
			printf("LASH ordered to restore data set\n");
			lash_send_event(lash_client, event);
			break;
		case LASH_Save_Data_Set:
			printf("LASH ordered to save data set\n");
			save_data();
			lash_send_event(lash_client, event);
			break;
		case LASH_Server_Lost:
			printf("LASH server lost\n");
			return 1;
		default:
			printf("%s: receieved unknown LASH event of type %d\n",
				   __FUNCTION__, lash_event_get_type(event));
			lash_event_destroy(event);
			break;
		}
	}

	while ((config = lash_get_config(lash_client))) {
		restore_data(config);
		lash_config_destroy(config);
	}

	return 0;
}

void *
lash_thread_main(void *data)
{
	printf("LASH thread running\n");

	while (!lash_main())
		usleep(1000);

	printf("LASH thread finished\n");
	return NULL;
}

/* EOF */
