/* adapted for jack and LASH by Bob Ham */

/* miniFMsynth 1.0 by Matthias Nagorni    */
/* This program uses callback-based audio */
/* playback as proposed by Paul Davis on  */
/* the linux-audio-dev mailinglist.       */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <jack/jack.h>
#include <alloca.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>

#include <lash/lash.h>

#define POLY 10

snd_seq_t *seq_handle;
double phi[POLY], phi_mod[POLY], pitch, modulation, velocity[POLY],
	attack, decay, sustain, release, env_time[POLY], env_level[POLY], gain,
	sample_rate = 48000.0;
int harmonic, subharmonic, transpose, note[POLY], gate[POLY],
	note_active[POLY];

int process(jack_nframes_t nframes, void *data);
int sample_rate_cb(jack_nframes_t sr, void *data);

lash_client_t *lash_client = NULL;

jack_client_t *jack_client = NULL;
jack_port_t *jack_port = NULL;

char jack_client_name[512];
char alsa_client_name[512];

int quit = 0;

snd_seq_t *
open_seq()
{
	snd_seq_t *seq_handle;

	if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		exit(1);
	}
	snd_seq_set_client_name(seq_handle, alsa_client_name);
	if (snd_seq_create_simple_port(seq_handle, "miniFMsynth",
								   SND_SEQ_PORT_CAP_WRITE |
								   SND_SEQ_PORT_CAP_SUBS_WRITE,
								   SND_SEQ_PORT_TYPE_APPLICATION |
								   SND_SEQ_PORT_TYPE_SYNTH) < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		exit(1);
	}

	printf("Connected to ALSA sequencer with client id %d\n",
		   snd_seq_client_id(seq_handle));
	lash_alsa_client_id(lash_client,
						(unsigned char)snd_seq_client_id(seq_handle));

	return (seq_handle);
}

void
open_jack()
{
	lash_event_t *event;

	jack_client = jack_client_open(jack_client_name, JackNullOption, NULL);
	if (!jack_client) {
		fprintf(stderr, "%s: could not connect to jack server, exiting\n",
				__FUNCTION__);
		exit(1);
	}
	printf("Connected to JACK with client name '%s'\n", jack_client_name);

	jack_port =
		jack_port_register(jack_client, "out_1", JACK_DEFAULT_AUDIO_TYPE,
						   JackPortIsOutput, 0);
	if (!jack_port) {
		fprintf(stderr, "%s: could not register jack port, exiting\n",
				__FUNCTION__);
		exit(1);
	}

	jack_set_process_callback(jack_client, process, NULL);

	sample_rate = jack_get_sample_rate(jack_client);
	jack_set_sample_rate_callback(jack_client, sample_rate_cb, NULL);

	jack_activate(jack_client);

	event = lash_event_new_with_type(LASH_Jack_Client_Name);
	lash_event_set_string(event, jack_client_name);
	lash_send_event(lash_client, event);

}

void
close_jack()
{
	jack_deactivate(jack_client);

}

double
envelope(int *note_active, int gate, double *env_level, double t,
		 double attack, double decay, double sustain, double release)
{

	if (gate) {
		if (t > attack + decay)
			return (*env_level = sustain);
		if (t > attack)
			return (*env_level =
					1.0 - (1.0 - sustain) * (t - attack) / decay);
		return (*env_level = t / attack);
	} else {
		if (t > release) {
			if (note_active)
				*note_active = 0;
			return (*env_level = 0);
		}
		return (*env_level * (1.0 - t / release));
	}
}

int
midi_callback()
{

	snd_seq_event_t *ev;
	int l1;

	do {
		snd_seq_event_input(seq_handle, &ev);
		switch (ev->type) {
		case SND_SEQ_EVENT_PITCHBEND:
			pitch = (double)ev->data.control.value / 8192.0;
			break;
		case SND_SEQ_EVENT_CONTROLLER:
			if (ev->data.control.param == 1) {
				modulation = (double)ev->data.control.value / 10.0;
			}
			break;
		case SND_SEQ_EVENT_NOTEON:
			if (ev->data.note.velocity > 0) {
				for (l1 = 0; l1 < POLY; l1++) {
					if (!note_active[l1]) {
						note[l1] = ev->data.note.note;
						velocity[l1] = ev->data.note.velocity / 127.0;
						env_time[l1] = 0;
						gate[l1] = 1;
						note_active[l1] = 1;
						break;
					}
				}
				break;
			}
		case SND_SEQ_EVENT_NOTEOFF:
			for (l1 = 0; l1 < POLY; l1++) {
				if (gate[l1] && note_active[l1]
					&& (note[l1] == ev->data.note.note)) {
					env_time[l1] = 0;
					gate[l1] = 0;
				}
			}
			break;
		}
		snd_seq_free_event(ev);
	} while (snd_seq_event_input_pending(seq_handle, 0) > 0);
	return (0);
}

int
sample_rate_cb(jack_nframes_t sr, void *data)
{
	sample_rate = sr;

	return 0;
}

void
go_real_time()
{
	pthread_t jack_thread;
	int policy;
	int set_priority = 0;
	struct sched_param rt_param;
	int err;

	memset(&rt_param, 0, sizeof(rt_param));

	jack_thread = jack_client_thread_id(jack_client);
	err = pthread_getschedparam(jack_thread, &policy, &rt_param);
	if (err)
		fprintf(stderr,
				"%s: could not get scheduling parameters of jack thread\n",
				__FUNCTION__);
	else {
		rt_param.sched_priority += 1;
		set_priority = 1;
	}

	/* make a guess; jackd runs at priority 10 by default I think */
	if (!set_priority) {
		memset(&rt_param, 0, sizeof(rt_param));
		rt_param.sched_priority = 15;
	}

	policy = SCHED_FIFO;

	err = pthread_setschedparam(pthread_self(), policy, &rt_param);
	if (err)
		fprintf(stderr, "%s: could not set SCHED_FIFO for midi thread: %s\n",
				__FUNCTION__, strerror(errno));

}

int
process(jack_nframes_t nframes, void *data)
{

	int l1, l2;
	double dphi, dphi_mod, f1, f2, f3, freq_note, sound;
	jack_default_audio_sample_t *jack_buffer;

	jack_buffer = jack_port_get_buffer(jack_port, nframes);

	memset(jack_buffer, 0, nframes * sizeof(jack_default_audio_sample_t));
	for (l2 = 0; l2 < POLY; l2++) {
		if (note_active[l2]) {
			f1 = 8.176 * exp((double)(transpose + note[l2] - 2) * log(2.0) /
							 12.0);
			f2 = 8.176 * exp((double)(transpose + note[l2]) * log(2.0) /
							 12.0);
			f3 = 8.176 * exp((double)(transpose + note[l2] + 2) * log(2.0) /
							 12.0);
			freq_note =
				(pitch > 0) ? f2 + (f3 - f2) * pitch : f2 + (f2 - f1) * pitch;
			dphi = M_PI * freq_note / (sample_rate / 2);
			dphi_mod = dphi * (double)harmonic / (double)subharmonic;
			for (l1 = 0; l1 < nframes; l1++) {
				phi[l2] += dphi;
				phi_mod[l2] += dphi_mod;
				if (phi[l2] > 2.0 * M_PI)
					phi[l2] -= 2.0 * M_PI;
				if (phi_mod[l2] > 2.0 * M_PI)
					phi_mod[l2] -= 2.0 * M_PI;
				sound =
					gain * envelope(&note_active[l2], gate[l2],
									&env_level[l2], env_time[l2], attack,
									decay, sustain, release)
					* velocity[l2] * sin(phi[l2] +
										 modulation * sin(phi_mod[l2]));
				env_time[l2] += 1.0 / sample_rate;

				jack_buffer[l1] += (sound / POLY);
			}
		}
	}
	return 0;
}

void *
synth_main(void *data)
{
	int seq_nfds, l1;
	struct pollfd *pfds;

	pitch = 0;
	for (l1 = 0; l1 < POLY; note_active[l1++] = 0) ;

	open_jack();
	seq_handle = open_seq();

	go_real_time();

	seq_nfds = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfds = (struct pollfd *)alloca(sizeof(struct pollfd) * seq_nfds);
	snd_seq_poll_descriptors(seq_handle, pfds, seq_nfds, POLLIN);

	printf("Synth running\n");

	while (!quit) {
		if (poll(pfds, seq_nfds, 1000) > 0) {
			for (l1 = 0; l1 < seq_nfds; l1++) {
				if (pfds[l1].revents > 0)
					midi_callback();
			}
		}

		if (lash_client && !lash_server_connected(lash_client))
			break;
	}

	snd_seq_close(seq_handle);
	close_jack();

	printf("Synth finished\n");
	return (NULL);
}
