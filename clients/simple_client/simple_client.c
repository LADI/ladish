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

#include <stdio.h>
#include <unistd.h>
#include <alloca.h>

#include <jack/jack.h>
#include <alsa/asoundlib.h>

#include <lash/lash.h>

char *
catdup(
	const char * str1,
	const char * str2)
{
	char * str;
	size_t str1_len;
	size_t str2_len;

	str1_len = strlen(str1);
	str2_len = strlen(str2);

	str = malloc(str1_len + str2_len + 1);
	if (str == NULL)
	{
		return NULL;
	}

	memcpy(str, str1, str1_len);
	memcpy(str + str1_len, str2, str2_len);
	str[str1_len + str2_len] = 0;

	return str;
}

int
main(int argc, char **argv)
{
	lash_event_t *event;
	lash_config_t *config;
	lash_client_t *client;
	jack_client_t *jack_client;
	snd_seq_t *aseq;
	snd_seq_client_info_t *aseq_client_info;
	char client_name[64];
	int err;
	int done = 0;

	sprintf(client_name, "lsec_%d", getpid());
	printf("%s: client name: '%s'\n", __FUNCTION__, client_name);

	printf("%s: attempting to initialise lash\n", __FUNCTION__);

	client = lash_init(lash_extract_args(&argc, &argv), "LASH Simple Client",
					   LASH_Config_Data_Set | LASH_Config_File,
					   LASH_PROTOCOL(2, 0));

	if (!client) {
		fprintf(stderr, "%s: could not initialise lash\n", __FUNCTION__);
		exit(1);
	}

	printf("%s: initialised\n", __FUNCTION__);

	event = lash_event_new_with_type(LASH_Client_Name);
	lash_event_set_string(event, client_name);
	lash_send_event(client, event);

	/*
	 * jack ports
	 */
	printf("%s: connecting to jack server\n", __FUNCTION__);
	jack_client = jack_client_open(client_name, JackNullOption, NULL);
	if (!jack_client) {
		printf("%s: could not connect to jack server", __FUNCTION__);
		exit(1);
	}
	printf("%s: connected to jack with client name '%s'\n", __FUNCTION__,
		   client_name);

	printf("%s: opening alsa sequencer\n", __FUNCTION__);
	err = snd_seq_open(&aseq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	if (err) {
		printf("%s: could not open alsa sequencer\n", __FUNCTION__);
		exit(1);
	}
	snd_seq_client_info_alloca(&aseq_client_info);
	snd_seq_get_client_info(aseq, aseq_client_info);
	snd_seq_client_info_set_name(aseq_client_info, client_name);
	snd_seq_set_client_info(aseq, aseq_client_info);
	printf("%s: opened alsa sequencer with id %d, name '%s'\n",
		   __FUNCTION__, snd_seq_client_id(aseq), client_name);

	event = lash_event_new_with_type(LASH_Jack_Client_Name);
	lash_event_set_string(event, client_name);
	lash_send_event(client, event);

	event = lash_event_new_with_type(LASH_Alsa_Client_ID);
	client_name[0] = (char)snd_seq_client_id(aseq);
	client_name[1] = '\0';
	printf("alsa client id: %d\n", snd_seq_client_id(aseq));
	lash_event_set_string(event, client_name);
	lash_send_event(client, event);

	while (!done) {
		printf("%s: trying to get events and configs\n", __FUNCTION__);

		event = lash_get_event(client);
		if (event) {
			printf("%s: got event of type %d with string '%s'\n",
				   __FUNCTION__, lash_event_get_type(event),
				   lash_event_get_string(event));
			switch (lash_event_get_type(event)) {
			case LASH_Save_Data_Set:
				config = lash_config_new_with_key("test config");
				lash_config_set_value_string(config,
											 "this is some configuration data");
				lash_send_config(client, config);
				lash_send_event(client, event);
				break;
			case LASH_Restore_Data_Set:
				while ((config = lash_get_config(client))) {
					printf("%s: got config with key '%s', value_size %d\n",
					       __FUNCTION__, lash_config_get_key(config),
					       lash_config_get_value_size(config));
					lash_config_free(config);
					free(config);
				}
				lash_send_event(client, event);
				break;
			case LASH_Save_File:
			{
				FILE *config_file;
				char *file_path;

				file_path = catdup(lash_event_get_string(event), "simple-client.data");
				config_file = fopen(file_path, "w");
				free(file_path);
				fprintf(config_file, "lash simple client data");
				fclose(config_file);
				printf("wrote config file\n");
				lash_send_event(client, event);
				break;
			}
			case LASH_Restore_File:
			{
				FILE *config_file;
				char *buf = NULL;
				size_t buf_size = 0;
				char *file_path;

				file_path = catdup(lash_event_get_string(event), "simple-client.data");
				config_file = fopen(file_path, "r");
				free(file_path);
				if (config_file) {
					getline(&buf, &buf_size, config_file);
					fclose(config_file);
				}

				printf("read data from config file: '%s'\n", buf);
				if (buf)
					free(buf);

				lash_send_event(client, event);
				break;
			}
			case LASH_Quit:
				printf("server told us to quit; exiting\n");
				done = 1;
				break;

			case LASH_Server_Lost:
				printf("server connection lost; exiting\n");
				exit(0);
				break;

			default:
				fprintf(stderr,
						"%s: recieved unknown LASH event of type %d\n",
						__FUNCTION__, lash_event_get_type(event));
				lash_event_destroy(event);
				break;
			}
		}

		sleep(5);
	}

	return 0;
}
