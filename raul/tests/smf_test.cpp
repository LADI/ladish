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

	cout << "Type: " << reader.type() << endl;
	cout << "Num tracks: " << reader.num_tracks() << endl;
	cout << "PPQN: " << reader.ppqn() << endl;

	unsigned char buf[4];
	uint32_t      ev_size;
	uint32_t      ev_delta_time;
	while (reader.read_event(4, buf, &ev_size, &ev_delta_time) >= 0) {

		cerr << "\n\nEvent, size = " << ev_size << ", time = " << ev_delta_time << endl;
		cerr << "Data: ";
		cerr.flags(ios::hex);
		for (uint32_t i=0; i < ev_size; ++i) {
			cerr << "0x" << (int)buf[i] << " ";
		}
		cerr.flags(ios::dec);
		cerr << endl;
	}

	return 0;
}
