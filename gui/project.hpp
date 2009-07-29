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
    const std::string& name);

  ~project();

  void
  clear();

  void
  get_name(
    std::string& name);

  void
  get_description(
    std::string& description);

  void
  get_notes(
    std::string& notes);

  bool
  get_modified_status();

  void
  get_clients(
    std::list<boost::shared_ptr<lash_client> >& clients);

  void
  do_rename(
    const std::string& name);

  void
  do_change_description(
    const std::string& description);

  void
  do_change_notes(
    const std::string& notes);

  sigc::signal0<void> _signal_renamed;
  sigc::signal0<void> _signal_modified_status_changed;
  sigc::signal0<void> _signal_description_changed;
  sigc::signal0<void> _signal_notes_changed;
  sigc::signal1<void, boost::shared_ptr<lash_client> > _signal_client_added;
  sigc::signal1<void, boost::shared_ptr<lash_client> > _signal_client_removed;

private:
  friend class lash_proxy_impl;

  void
  on_name_changed(
    const std::string& name);

  void
  on_modified_status_changed(
    bool modified_status);

  void
  on_description_changed(
    const std::string& description);

  void
  on_notes_changed(
    const std::string& notes);

  void
  on_client_added(
    boost::shared_ptr<lash_client> client_ptr);

  void
  on_client_removed(
    const std::string& id);

  project_impl * _impl_ptr;
};

#endif // #ifndef PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED
