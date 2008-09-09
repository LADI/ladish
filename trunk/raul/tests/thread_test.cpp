#include <iostream>
#include <raul/Thread.hpp>

using namespace std;
using namespace Raul;

int
main()
{
	Thread& this_thread = Thread::get();
	this_thread.set_name("Main");

	cout << "Thread name should be Main" << endl;

	cout << "Thread name: " << Thread::get().name() << endl;

	return 0;
}

