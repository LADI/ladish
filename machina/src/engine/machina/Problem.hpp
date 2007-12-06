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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_PROBLEM_HPP
#define MACHINA_PROBLEM_HPP

#include <raul/MIDISink.hpp>

namespace Machina {

class Machine;


class Problem {
public:
	Problem(const std::string& target_midi);

	float fitness(Machine& machine);

private:
	struct Evaluator : public Raul::MIDISink {
		Evaluator(Problem& problem) : _problem(problem), _n_notes(0) {
			for (uint8_t i=0; i < 128; ++i)
				_note_frequency[i] = 0;
		}
		void write_event(Raul::BeatTime time,
		                 size_t         ev_size,
		                 const uint8_t* ev) throw (std::logic_error);
		void compute();
		Problem& _problem;
	
		float  _note_frequency[128];
		size_t _n_notes;
	};

	Evaluator _target;
};


} // namespace Machina

#endif // MACHINA_PROBLEM_HPP
