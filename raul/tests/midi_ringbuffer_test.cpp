#include "raul/TimeStamp.hpp"
#include "raul/EventRingBuffer.hpp"
#include <iostream>
#include "raul/midi_names.h"

using namespace std;
using namespace Raul;

void
read_write_test(EventRingBuffer& rb, unsigned offset)
{
	TimeStamp     t(TimeUnit(TimeUnit::FRAMES, 48000), 0, 0);
	size_t        size;
	unsigned char buf[5];

	snprintf((char*)buf, 5, "%d", offset);
	size = strlen((char*)buf);

	size_t written = rb.write(t, size, buf);
	assert(written == size);
	
	for (size_t i=0; i < 4; ++i)
		buf[i] = 0;

	rb.read(&t, &size, buf);
	
	cout << "t=" << t << ", s=" << size << ", b='" << buf << "'" << endl;

}

	
int
main()
{
	EventRingBuffer rb(32);

	for (size_t i=0; i < 9999; ++i)
		read_write_test(rb, i);

	return 0;
}

