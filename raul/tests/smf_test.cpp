#include <iostream>
#include <string>
#include <raul/SMFReader.h>
#include <raul/SMFWriter.h>

using namespace std;
using namespace Raul;


int
main(int argc, char** argv)
{
	char* filename = NULL;

	if (argc < 2) {
		filename = "./test.mid";
		SMFWriter writer(32768);
		writer.start(string(filename));
		writer.finish();
		cout << "Wrote " << filename << " with PPQN = " << writer.ppqn() << endl;

	} else {
		filename = argv[1];
	}


	SMFReader reader;
	bool opened = reader.open(filename);

	if (!opened) {
		cerr << "Unable to open SMF file " << filename << endl;
		return -1;
	}

	cout << "Opened SMF file " << filename << endl;

	cout << "Num tracks: " << reader.num_tracks() << endl;
	cout << "PPQN: " << reader.ppqn() << endl;

	return 0;
}
