#include <iostream>
#include "raul/MIDIRingBuffer.h"

using namespace std;
using namespace Raul;

void
read_write_test(MIDIRingBuffer& rb, unsigned offset)
{
	TickTime t;
	size_t   size;
	char     buf[5];

	snprintf(buf, 5, "%d", offset);
	size = strlen(buf);

	size_t written = rb.write(offset, size, buf);
	assert(written == size);
	
	for (size_t i=0; i < 4; ++i)
		buf[i] = 0;

	rb.read(&t, &size, buf);
	
	cout << "t=" << t << ", s=" << size << ", b='" << buf << "'" << endl;

}

	
int
main()
{
	MIDIRingBuffer rb(32);

	for (size_t i=0; i < 9999; ++i)
		read_write_test(rb, i);

	return 0;
}

