/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <raul/Quantizer.h>
#include <machina/LearnRequest.hpp>

namespace Machina {


/** Add the learned actions to the node */
void
LearnRequest::finish(BeatTime time)
{
	_node->set_enter_action(_enter_action);
	_node->set_exit_action(_exit_action);

	double duration = Raul::Quantizer::quantize(_quantization, time - _start_time);
	
	_node->set_duration(duration);
	using namespace std;
	cerr << "Q=" << _quantization << ", T=" << time << ", ST=" << _start_time << endl;
	std::cerr << "LEARN DURATION: " << duration << std::endl;
}


}
