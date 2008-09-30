/* An LV2 plugin which transposes MIDI notes.
 * Copyright (C) 2008 Dave Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "ext/lv2plugin.hpp"
#include "ext/event/lv2_event.h"

using namespace std;

class Transpose : public LV2::Plugin< Transpose, LV2::UriMapExt<true> > {
public:
	Transpose(double rate)
		: LV2::Plugin< Transpose, LV2::UriMapExt<true> >(3)
		, _midi_event(uri_to_id(LV2_EVENT_URI, "http://lv2plug.in/ns/ext/midi"))
		, _osc_event(uri_to_id(LV2_EVENT_URI, "http://lv2plug.in/ns/ext/osc"))
	{
	}

	void run(uint32_t sample_count) {
		LV2_Event_Buffer* in = p<LV2_Event_Buffer>(0);
		if (in->event_count > 0)
			cout << "Transpose received " << in->event_count << " events."  << endl;
	}

private:
	uint32_t _midi_event;
	uint32_t _osc_event;
};

static unsigned _ = Transpose::register_class(
		"http://drobilla.net/lv2_plugins/dev/transpose");
