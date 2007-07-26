#include <iostream>
#include <raul/TimeSlice.hpp>

using namespace std;
using namespace Raul;


int
main()
{
	TimeSlice ts(1/48000.0, 120);

	double in_double = 0;

	cout << "Beats: ";
	cin >> in_double;

	cout << "\tSeconds: ";
	cout << ts.beats_to_seconds(in_double);
	cout << endl;
	
	cout << "\tTicks:   ";
	cout << ts.beats_to_ticks(in_double);
	cout << endl;

	return 0; 
}

