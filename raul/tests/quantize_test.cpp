#include <iostream>
#include <raul/Quantizer.h>

using namespace std;
using namespace Raul;


int
main()
{
	double q = 0;
	double beats = 0;

	cout << "Quantization: ";
	cin >> q;
	cout << endl;

	while (true) {
		cout << "Beats: ";
		cin >> beats;

		cout << "Quantized: ";
		cout << Quantizer::quantize(q, beats) << endl << endl;
	}

	return 0; 
}

