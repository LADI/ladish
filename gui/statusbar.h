/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the statusbar helpers
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

#ifndef STATUSBAR_H__6A47F295_8858_4611_80D0_034A8FB3802A__INCLUDED
#define STATUSBAR_H__6A47F295_8858_4611_80D0_034A8FB3802A__INCLUDED

#include "common.h"

void init_statusbar(void);

void set_studio_status_text(const char * text);
GtkImage * get_status_image(void);

void set_latency_text(const char * text);
void clear_latency_text(void);

void set_dsp_load_text(const char * text);
void clear_dsp_load_text(void);

void set_xruns_text(const char * text);
void clear_xruns_text(void);

void set_sample_rate_text(const char * text);
void clear_sample_rate_text(void);

#endif /* #ifndef STATUSBAR_H__6A47F295_8858_4611_80D0_034A8FB3802A__INCLUDED */
