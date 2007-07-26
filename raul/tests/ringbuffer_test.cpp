#include <iostream>
#include "raul/RingBuffer.hpp"

using namespace std;
using namespace Raul;

void
print_buf(size_t size, char* buf)
{
	cout << "{ ";
	for (size_t i=0; i < size; ++i) {
		cout << buf[i];
		if (i < size-1)
			cout << ", ";
	}

	cout << " }" << endl;
}


int
main()
{
	RingBuffer<char> rb(5);
	
	char ev[] = { 'a', 'b', 'c' };

	rb.write(3, ev);

	char buf[3];
	rb.read(3, buf);
	print_buf(3, buf);

	char ev2[] = { 'd', 'e', 'f' };
	rb.write(3, ev2);
	

	size_t read = rb.read(3, buf);
	if (read < 3)
		rb.read(3 - read, buf + read);

	print_buf(3, buf);

	return 0;
}

