/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the session class
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

#include "common.h"
#include "project.hpp"
#include "session.hpp"
#include "lash_client.hpp"

struct session_impl
{
  std::list<boost::shared_ptr<project> > projects;
  std::list<boost::shared_ptr<lash_client> > clients;
};

session::session()
{
  _impl_ptr = new session_impl;
}

session::~session()
{
  delete _impl_ptr;
}

void
session::clear()
{
  boost::shared_ptr<project> project_ptr;

  _impl_ptr->clients.clear();

  while (!_impl_ptr->projects.empty())
  {
    project_ptr = _impl_ptr->projects.front();
    _impl_ptr->projects.pop_front();
    project_ptr->clear();
    _signal_project_closed.emit(project_ptr);
  }
}

void
session::project_add(
  boost::shared_ptr<project> project_ptr)
{
  _impl_ptr->projects.push_back(project_ptr);

  _signal_project_added.emit(project_ptr);
}

boost::shared_ptr<project>
session::find_project_by_name(
  const std::string& name)
{
  boost::shared_ptr<project> project_ptr;
  std::string temp_name;

  for (std::list<boost::shared_ptr<project> >::iterator iter = _impl_ptr->projects.begin(); iter != _impl_ptr->projects.end(); iter++)
  {
    project_ptr = *iter;
    project_ptr->get_name(temp_name);

    if (temp_name == name)
    {
      return project_ptr;
    }
  }

  return boost::shared_ptr<project>();
}

void
session::project_close(
  const std::string& project_name)
{
  boost::shared_ptr<project> project_ptr;
  std::string temp_name;
  std::list<boost::shared_ptr<lash_client> > clients;

  for (std::list<boost::shared_ptr<project> >::iterator iter = _impl_ptr->projects.begin(); iter != _impl_ptr->projects.end(); iter++)
  {
    project_ptr = *iter;
    project_ptr->get_name(temp_name);

    if (temp_name == project_name)
    {
      _impl_ptr->projects.erase(iter);
      _signal_project_closed.emit(project_ptr);

      // remove clients from session, if not removed already
      project_ptr->get_clients(clients);
      for (std::list<boost::shared_ptr<lash_client> >::iterator iter = clients.begin(); iter != clients.end(); iter++)
      {
        std::string id;

        (*iter)->get_id(id);

        client_remove(id);
      }

      return;
    }
  }
}

void
session::client_add(
  boost::shared_ptr<lash_client> client_ptr)
{
  _impl_ptr->clients.push_back(client_ptr);
}

void
session::client_remove(
  const std::string& id)
{
  boost::shared_ptr<lash_client> client_ptr;
  std::string temp_id;

  for (std::list<boost::shared_ptr<lash_client> >::iterator iter = _impl_ptr->clients.begin(); iter != _impl_ptr->clients.end(); iter++)
  {
    client_ptr = *iter;
    client_ptr->get_id(temp_id);

    if (temp_id == id)
    {
      _impl_ptr->clients.erase(iter);
      return;
    }
  }
}

boost::shared_ptr<lash_client>
session::find_client_by_id(const std::string& id)
{
  boost::shared_ptr<lash_client> client_ptr;
  std::string temp_id;

  for (std::list<boost::shared_ptr<lash_client> >::iterator iter = _impl_ptr->clients.begin(); iter != _impl_ptr->clients.end(); iter++)
  {
    client_ptr = *iter;
    client_ptr->get_id(temp_id);

    if (temp_id == id)
    {
      return client_ptr;
    }
  }

  return boost::shared_ptr<lash_client>();
}
