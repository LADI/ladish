#include <raul/TimeStamp.hpp>
#include <raul/TimeSlice.hpp>
#include <iostream>

using namespace std;
using namespace Raul;


int
main()
{
	TimeUnit  unit(TimeUnit::BEATS, 19200);
	TimeSlice ts(48000, 19200, 120.0);

	double in_double;
	cout << "Beats: ";
	cin >> in_double;
	
	TimeStamp t(unit, (uint32_t)in_double,
			(uint32_t)((in_double - (uint32_t)in_double) * unit.ppt()));

	cout << "\tSeconds: ";
	cout << ts.beats_to_seconds(t);
	cout << endl;
	
	cout << "\tTicks:   ";
	cout << ts.beats_to_ticks(t);
	cout << endl;

	return 0; 
}

