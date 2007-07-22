#include <iostream>
#include "raul/StampedChunkRingBuffer.h"
#include "raul/midi_names.h"

using namespace std;
using namespace Raul;

void
read_write_test(StampedChunkRingBuffer& rb, unsigned offset)
{
	TickTime      t;
	size_t        size;
	unsigned char buf[5];

	snprintf((char*)buf, 5, "%d", offset);
	size = strlen((char*)buf);

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
	StampedChunkRingBuffer rb(32);

	for (size_t i=0; i < 9999; ++i)
		read_write_test(rb, i);

	return 0;
}

