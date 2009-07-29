/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *
 **************************************************************************
 * This file implements the canvas port class
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

#ifndef PATCHAGE_PATCHAGEPORT_HPP
#define PATCHAGE_PATCHAGEPORT_HPP

#include "common.h"

enum PortType { JACK_AUDIO, JACK_MIDI };

/** A Port on a PatchageModule
 *
 * \ingroup OmGtk
 */
struct PatchagePort
{
  PatchagePort(
    canvas_module_handle module,
    const std::string& name,
    bool is_input,
    int color);
  virtual ~PatchagePort();

  PortType type;
  bool is_a2j_mapped;
  std::string a2j_jack_port_name; // valid only if is_a2j_mapped is true
};


#endif // PATCHAGE_PATCHAGEPORT_HPP
