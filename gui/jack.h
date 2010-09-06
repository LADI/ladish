/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the JACK related functionality
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

#ifndef JACK_H__AA9BB099_1EAA_43A8_B84D_3DA221F1A1CF__INCLUDED
#define JACK_H__AA9BB099_1EAA_43A8_B84D_3DA221F1A1CF__INCLUDED

/* JACK states */
#define JACK_STATE_NA         0
#define JACK_STATE_STOPPED    1
#define JACK_STATE_STARTED    2

unsigned int get_jack_state(void);
void buffer_size_clear(void);
void init_jack_widgets(void);
bool init_jack(void);
void uninit_jack(void);
bool jack_xruns(void);
void set_xrun_progress_bar_text(const char * text);
void update_jack_sample_rate(void);
void clear_xruns_and_max_dsp(void);

#endif /* #ifndef JACK_H__AA9BB099_1EAA_43A8_B84D_3DA221F1A1CF__INCLUDED */
