/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface of the project class
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

#ifndef PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED
#define PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED

struct project_impl;
class lash_proxy;
class lash_proxy_impl;
class lash_client;

class project
{
public:
  project(
    lash_proxy * lash_ptr,
    const string& name);

  ~project();

  void
  clear();

  void
  get_name(
    string& name);

  void
  get_description(
    string& description);

  void
  get_notes(
    string& notes);

  bool
  get_modified_status();

  void
  get_clients(
    list<shared_ptr<lash_client> >& clients);

  void
  do_rename(
    const string& name);

  void
  do_change_description(
    const string& description);

  void
  do_change_notes(
    const string& notes);

  signal0<void> _signal_renamed;
  signal0<void> _signal_modified_status_changed;
  signal0<void> _signal_description_changed;
  signal0<void> _signal_notes_changed;
  signal1<void, shared_ptr<lash_client> > _signal_client_added;
  signal1<void, shared_ptr<lash_client> > _signal_client_removed;

private:
  friend class lash_proxy_impl;

  void
  on_name_changed(
    const string& name);

  void
  on_modified_status_changed(
    bool modified_status);

  void
  on_description_changed(
    const string& description);

  void
  on_notes_changed(
    const string& notes);

  void
  on_client_added(
    shared_ptr<lash_client> client_ptr);

  void
  on_client_removed(
    const string& id);

  project_impl * _impl_ptr;
};

#endif // #ifndef PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED
