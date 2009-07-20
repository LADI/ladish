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

#ifndef __LASHD_DBUS_IFACE_CONTROL_H__
#define __LASHD_DBUS_IFACE_CONTROL_H__

extern const interface_t g_lashd_interface_control;

void
lashd_dbus_signal_emit_project_appeared(
  const char * project_name,
  const char * project_path);

void
lashd_dbus_signal_emit_project_disappeared(
  const char * project_name);

void
lashd_dbus_signal_emit_project_name_changed(
  const char * old_name,
  const char * new_name);

void
lashd_dbus_signal_emit_project_path_changed(
  const char * project_name,
  const char * new_path);

void
lashd_dbus_signal_emit_project_description_changed(
  const char * project_name,
  const char * description);

void
lashd_dbus_signal_emit_project_notes_changed(
  const char * project_name,
  const char * notes);

void
lashd_dbus_signal_emit_project_saved(
  const char * project_name);

void
lashd_dbus_signal_emit_project_loaded(
  const char * project_name);

void
lashd_dbus_signal_emit_client_appeared(
  const char * client_id,
  const char * project_name,
  const char * client_name);

void
lashd_dbus_signal_emit_client_disappeared(
  const char * client_id,
  const char * project_name);

void
lashd_dbus_signal_emit_client_name_changed(
  const char * client_id,
  const char * new_client_name);

void
lashd_dbus_signal_emit_progress(
  uint8_t percentage);

#endif /* __LASHD_DBUS_IFACE_CONTROL_H__ */
