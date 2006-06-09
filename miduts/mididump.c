/* JACK MIDI Dump
 * Copyright (C) 2006 Dave Robillard
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <jack/jack.h>
#include <jack/midiport.h>

jack_client_t* client;
jack_port_t*   input_port;
jack_nframes_t current_frame; // frame at start of current cycle (counter)
jack_nframes_t last_event_timestamp;
jack_nframes_t sample_rate;

double
frames_to_seconds(jack_nframes_t frames)
{
	return frames / (double)sample_rate;
}

void
print_midi_message(unsigned char* buf, size_t len)
{	
	if (buf[0] == 0xF0 && buf[1] == 0x7F) {
		printf("MTC Full Frame (%hhu:%hhu:%hhu:%hhu)\n"
			, buf[5] &0xF, buf[6], buf[7], buf[8]);
	} else if (buf[0] == 0xF1) {
		printf("MTC Quarter Frame %d\n", (buf[1] & 0xF0) >> 4);
	} else {
		printf("Unknown Raw: ");
		for (int i=0; i < len; ++i)
			printf("0x%X ", buf[i] & 0xFF);
		printf("\n");
	}
}

int
process(jack_nframes_t nframes, void *arg)
{
	void *port_buf = jack_port_get_buffer(input_port, nframes);
	jack_midi_event_t in_event;
	jack_nframes_t event_count =
		jack_midi_port_get_info(port_buf, nframes)->event_count;
	
	jack_position_t pos;
	jack_transport_query(client, &pos);
	
	current_frame = pos.frame;

	for (int i=0; i < event_count; ++i) {
		jack_midi_event_get(&in_event, port_buf, i, nframes);
	
		print_midi_message(in_event.buffer, in_event.size);
		
		jack_nframes_t timestamp = current_frame + in_event.time;
		printf("\tFrames since last: %u", timestamp - last_event_timestamp);
		printf("\t(%7.2lf ms)\n",
			frames_to_seconds(timestamp - last_event_timestamp) * 1000.0);
		last_event_timestamp = timestamp;
		
	//	printf("\n");
	}

	return 0;
}

int
srate(jack_nframes_t nframes, void *arg)
{
	sample_rate = nframes;
	printf("Sample rate: %" PRIu32 " Hz\n", nframes);
	return 0;
}

void
jack_shutdown(void *arg)
{
	exit(1);
}

int
main(int narg, char **args)
{
	current_frame = 0;

	if ((client = jack_client_new("mididump")) == 0) {
		fprintf(stderr, "jack server not running?\n");
		return 1;
	}

	jack_set_process_callback(client, process, 0);

	jack_set_sample_rate_callback(client, srate, 0);

	jack_on_shutdown(client, jack_shutdown, 0);

	input_port =
		jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE,
						   JackPortIsInput, 0);

	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
		return 1;
	}

	/* run until interrupted */
	while (1) {
		sleep(1);
	}
	jack_client_close(client);
	exit(0);
}
