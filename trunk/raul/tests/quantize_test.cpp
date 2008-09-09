#include <raul/Quantizer.hpp>
#include <iostream>

using namespace std;
using namespace Raul;


int
main()
{
	double in = 0;

	cout << "Quantization: ";
	cin >> in;
	cout << endl;
	
	TimeStamp q(TimeUnit(TimeUnit::BEATS, 19200), in);

	while (true) {
		cout << "Beats: ";
		cin >> in;
	
		TimeStamp beats(TimeUnit(TimeUnit::BEATS, 19200), in);

		cout << "Quantized: ";
		cout << Quantizer::quantize(q, beats) << endl << endl;
	}

	return 0; 
}

