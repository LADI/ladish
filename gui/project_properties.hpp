/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface of the project_properties_dialog class
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

#ifndef PROJECT_PROPERTIES_HPP__B854A265_3660_4DAA_87E7_104020C5962D__INCLUDED
#define PROJECT_PROPERTIES_HPP__B854A265_3660_4DAA_87E7_104020C5962D__INCLUDED

struct project_properties_dialog_impl;

class project_properties_dialog
{
public:
  project_properties_dialog();
  ~project_properties_dialog();

  void run(boost::shared_ptr<project> project_ptr);

private:
  project_properties_dialog_impl * _impl_ptr;
};

#endif // #ifndef PROJECT_PROPERTIES_HPP__B854A265_3660_4DAA_87E7_104020C5962D__INCLUDED
