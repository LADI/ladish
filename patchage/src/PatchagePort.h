/* This file is part of Patchage.  Copyright (C) 2004 Dave Robillard.
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PATCHAGEPORT_H
#define PATCHAGEPORT_H

#include "config.h"
#include <string>
#include <list>
#include <flowcanvas/Port.h>
#include <flowcanvas/Module.h>
#include <boost/shared_ptr.hpp>

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

using namespace LibFlowCanvas;
using std::string; using std::list;

enum PortType { JACK_AUDIO, JACK_MIDI, ALSA_MIDI };


/** A Port on a PatchageModule
 *
 * \ingroup OmGtk
 */
class PatchagePort : public LibFlowCanvas::Port
{
public:
	PatchagePort(boost::shared_ptr<Module> module, PortType type, const string& name, bool is_input, int color)
	: Port(module, name, is_input, color),
	  m_type(type)
	{
#ifdef HAVE_ALSA
		m_alsa_addr.client = '\0';
		m_alsa_addr.port = '\0';
#endif
	}

	virtual ~PatchagePort() {}

#ifdef HAVE_ALSA
	// FIXME: This driver specific crap really needs to go
	void                  alsa_addr(const snd_seq_addr_t addr) { m_alsa_addr = addr; }
	const snd_seq_addr_t* alsa_addr() const
	{ return (m_type == ALSA_MIDI) ? &m_alsa_addr : NULL; }
#endif
	/** Returns the full name of this port, as "modulename:portname" */
	string full_name() const { return m_module.lock()->name() + ":" + m_name; }

	PortType type() const { return m_type; }

private:
#ifdef HAVE_ALSA
	snd_seq_addr_t m_alsa_addr;
#endif
	PortType       m_type;
};


#endif // PATCHAGEPORT_H
