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

#define __STDC_LIMIT_MACROS 1

#include <stdint.h>
#include <set>
#include <vector>
#include <iostream>
#include <machina/Problem.hpp>
#include <machina/Machine.hpp>
#include <machina/ActionFactory.hpp>
#include <machina/Edge.hpp>
#include <raul/SMFReader.hpp>
#include <raul/midi_events.h>
	#include <eugene/core/Problem.hpp>

using namespace std;

namespace Machina {

	
Problem::Problem(const std::string& target_midi, SharedPtr<Machine> seed)
	: _target(*this)
	, _seed(new Machine(*seed.get()))
{
	Raul::SMFReader smf;
	const bool opened = smf.open(target_midi);
	assert(opened);
	
	smf.seek_to_track(2); // FIXME: ?
	
	uint8_t  buf[4];
	uint32_t ev_size;
	uint32_t delta_time;
	while (smf.read_event(4, buf, &ev_size, &delta_time) >= 0) {
		// time ignored
		_target.write_event(0, ev_size, buf);
#if 0
		//_target._length += delta_time / (double)smf.ppqn();
		if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
			const uint8_t note = buf[1];
			/*++_target._note_frequency[note];
			++_target.n_notes();*/
			++_target._counts[note];
			_target._notes.push_back(note);
		}
#endif
	}

	cout << "Target notes: " << _target.n_notes() << endl;

	_target.compute();
}


float
Problem::fitness(const Machine& const_machine) const
{
	//cout << "(";

	// kluuudge
	Machine& machine = const_cast<Machine&>(const_machine);

	map<Machine*, float>::const_iterator cached = _fitness.find(&machine);
	if (cached != _fitness.end())
		return cached->second;
	
	SharedPtr<Evaluator> eval(new Evaluator(*this));

	machine.reset(0.0f);
	machine.deactivate();
	machine.activate();
	machine.set_sink(eval);

	// FIXME: timing stuff here isn't right at all...
	
	static const unsigned ppqn = 19200;
	Raul::TimeSlice time(1.0/(double)ppqn, 120);
	time.set_start(0);
	time.set_length(2*ppqn);
		
	machine.run(time);
	if (eval->n_notes() == 0)
		return 0.0f; // bad dog
	
	time.set_start(time.start_ticks() + 2*ppqn);

	while (eval->n_notes() < _target.n_notes()) {
		machine.run(time);
		if (machine.is_finished())
			machine.reset(time.start_ticks());
		time.set_start(time.start_ticks() + 2*ppqn);
	}

	eval->compute();
	
	// count
#if 0
	float f = 0;

	for (uint8_t i=0; i < 128; ++i) {
		/*if (eval->_note_frequency[i] <= _target._note_frequency[i])
			f += eval->_note_frequency[i];
		else
			f -= _target._note_frequency[i] - eval->_note_frequency[i];*/
		//f -= fabs(eval->_note_frequency[i] - _target._note_frequency[i]);
	}
#endif
	//cout << ")";
	
	// distance
	//float f = distance(eval->_notes, _target._notes);
	
	float f = 0.0;
	
	for (Evaluator::Patterns::const_iterator i = eval->_patterns.begin(); i != eval->_patterns.end(); ++i) {
		// Reward for matching patterns
		if (_target._patterns.find(i->first) != _target._patterns.end()) {
			Evaluator::Patterns::const_iterator c = _target._patterns.find(i->first);
			const uint32_t cnt = (c == _target._patterns.end()) ? 1 : c->second;
			f += min(i->second, cnt) * (i->first.length());

		// Punish for bad patterns
		} else {
			const uint32_t invlen = (eval->_order - i->first.length() + 1);
			f -= (i->second / (float)eval->_patterns.size() * (float)(invlen*invlen*invlen)) * 4;
		}
	}
	
	// Punish for missing patterns
	for (Evaluator::Patterns::const_iterator i = _target._patterns.begin(); i != _target._patterns.end(); ++i) {
		if (eval->_patterns.find(i->first) == _target._patterns.end()) {
				f -= i->second / (float)_target.n_notes() * (float)(eval->_order - i->first.length() + 1);
		}
	}

	//cout << "f = " << f << endl;

	_fitness[&machine] = f;

	return f;
}
	

void
Problem::Evaluator::write_event(Raul::BeatTime time,
                                size_t         ev_size,
                                const uint8_t* ev) throw (std::logic_error)
{
	if ((ev[0] & 0xF0) == MIDI_CMD_NOTE_ON) {

		const uint8_t note = ev[1];
		
		if (_first_note == 0)
			_first_note = note;
		
		/*++_note_frequency[note];
		++n_notes();*/
		//_notes.push_back(note);
		if (_read.length() == 0) {
			_read = note;
			return;
		}
		if (_read.length() == _order)
			_read = _read.substr(1);

		_read = _read + (char)note;

		for (size_t i = 0; i < _read.length(); ++i) {
			const string pattern = _read.substr(i);
			Patterns::iterator i = _patterns.find(pattern);
			if (i != _patterns.end())
				++(i->second);
			else
				_patterns[pattern] = 1;
		}

		++_counts[note];
		++_n_notes;
		
	}
}


void
Problem::Evaluator::compute()
{
	/*
	for (uint8_t i=0; i < 128; ++i) {
		if (_note_frequency[i] > 0) {
			_note_frequency[i] /= (float)n_notes();
			//cout << (int)i << ":\t" << _note_frequency[i] << endl;
		}
	}*/
}
	

boost::shared_ptr<Problem::Population>
Problem::initial_population(size_t gene_size, size_t pop_size) const
{
	boost::shared_ptr<Population> ret(new std::vector<Machine>());

	// FIXME: ignores _seed and builds based on MIDI
	// evolution of the visible machine would be nice..
	SharedPtr<Machine> base = SharedPtr<Machine>(new Machine());
	for (uint8_t i=0; i < 128; ++i) {
		if (_target._counts[i] > 0) {
			//cout << "Initial note: " << (int)i << endl;
			SharedPtr<Node> node(new Node(1/2.0));
			node->set_enter_action(ActionFactory::note_on(i));
			node->set_exit_action(ActionFactory::note_off(i));
			node->set_selector(true);
			base->add_node(node);
		}
	}

	for (size_t i = 0; i < pop_size; ++i) {
		// FIXME: double copy
		Machine m(*base.get());

		set< SharedPtr<Node> > unreachable;
		SharedPtr<Node> initial;

		for (Machine::Nodes::iterator i = m.nodes().begin(); i != m.nodes().end(); ++i) {
			if (PtrCast<MidiAction>((*i)->enter_action())->event()[1] == _target.first_note()) {
				(*i)->set_initial(true);
				initial = *i;
			} else {
				unreachable.insert(*i);
			}
		}

		SharedPtr<Node> cur = initial;
		unreachable.erase(cur);
		SharedPtr<Node> head;

		while ( ! unreachable.empty()) {
			if (rand() % 2)
				head = m.random_node();
			else
				head = *unreachable.begin();

			if ( ! head->connected_to(head) ) {
				cur->add_edge(SharedPtr<Edge>(new Edge(cur, head)));
				unreachable.erase(head);
				cur = head;
			}
		}
		
		ret->push_back(m);

		/*cout << "initial # nodes: " << m.nodes().size();
		cout << "initial fitness: " << fitness(m) << endl;*/
	}

	return ret;
}


/** levenshtein distance (edit distance) */
size_t
Problem::distance(const std::vector<uint8_t>& source,
                  const std::vector<uint8_t>& target) const
{
	// Derived from http://www.merriampark.com/ldcpp.htm
	
	assert(source.size() < UINT16_MAX);
	assert(target.size() < UINT16_MAX);
	
	// Step 1

	const uint16_t n = source.size();
	const uint16_t m = target.size();
	if (n == 0)
		return m;
	if (m == 0)
		return n;

	_matrix.resize(n + 1);
	for (uint16_t i = 0; i <= n; i++)
		_matrix[i].resize (m + 1);

	// Step 2

	for (uint16_t i = 0; i <= n; i++)
		_matrix[i][0] = i;

	for (uint16_t j = 0; j <= m; j++)
		_matrix[0][j] = j;

	// Step 3

	for (uint16_t i = 1; i <= n; i++) {

		const uint8_t s_i = source[i - 1];

		// Step 4

		for (uint16_t j = 1; j <= m; j++) {

			const uint8_t t_j = target[j - 1];

			// Step 5

			uint16_t cost;
			if (s_i == t_j) {
				cost = 0;
			} else {
				cost = 1;
			}

			// Step 6

			const uint16_t above = _matrix[i - 1][j];
			const uint16_t left = _matrix[i][j - 1];
			const uint16_t diag = _matrix[i - 1][j - 1];
			uint16_t cell = min (above + 1, min (left + 1, diag + cost));

			// Step 6A: Cover transposition, in addition to deletion,
			// insertion and substitution. This step is taken from:
			// Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
			// Enhanced Dynamic Programming ASM Algorithm"
			// (http://www.acm.org/~hlb/publications/asm/asm.html)

			if (i > 2 && j > 2) {
				uint16_t trans = _matrix[i - 2][j - 2] + 1;
				if (source[i - 2] != t_j)
					trans++;
				if (s_i != target[j - 2])
					trans++;
				if (cell > trans)
					cell = trans;
			}

			_matrix[i][j] = cell;
		}
	}

	// Step 7

	return _matrix[n][m];
}


} // namespace Machina

