/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __LASHD_TYPES_H__
#define __LASHD_TYPES_H__

typedef struct _server server_t;

#ifdef HAVE_JACK_DBUS
typedef struct _lashd_jackdbus_mgr lashd_jackdbus_mgr_t;
#else
typedef struct _jack_mgr jack_mgr_t;
#endif /* HAVE_JACK_DBUS */

typedef struct _jack_fport jack_fport_t;

typedef struct _jack_patch jack_patch_t;

typedef struct _jack_mgr_client jack_mgr_client_t;

#ifdef HAVE_ALSA
typedef struct _alsa_mgr alsa_mgr_t;

typedef struct _alsa_client alsa_client_t;

typedef struct _alsa_fport alsa_fport_t;

typedef struct _alsa_patch alsa_patch_t;
#endif

typedef struct _project project_t;

typedef struct _store store_t;

typedef struct _client_dependency client_dependency_t;

#endif /* __LASHD_TYPES_H__ */
