/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *
 **************************************************************************
 * This file contains interface of the canvas class
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

#ifndef PATCHAGE_PATCHAGECANVAS_HPP
#define PATCHAGE_PATCHAGECANVAS_HPP

#include "common.h"

class Patchage;
class PatchageModule;
class PatchagePort;

class PatchageCanvas
{
public:
  PatchageCanvas(Patchage* _app, int width, int height);
  
  boost::shared_ptr<PatchageModule> find_module(const std::string& name, ModuleType type);
  
  void connect(boost::shared_ptr<PatchagePort> port1, boost::shared_ptr<PatchagePort> port2);
  void disconnect(boost::shared_ptr<PatchagePort> port1, boost::shared_ptr<PatchagePort> port2);

  void status_message(const std::string& msg);
  
  boost::shared_ptr<PatchagePort>
  get_port(
    const std::string& module_name,
    const std::string& port_name);

  boost::shared_ptr<PatchagePort>
  lookup_port_by_a2j_jack_port_name(
    const char * a2j_jack_port_name);

private:
  Patchage* _app;
  canvas_handle _canvas;
};


#endif // PATCHAGE_PATCHAGECANVAS_HPP
