#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include "raul/SRSWQueue.hpp"
#include "raul/SRMWQueue.hpp"
#include "raul/Thread.hpp"
#include "raul/AtomicInt.hpp"

using namespace std;
using namespace Raul;

static const unsigned NUM_DATA = 10;
static const unsigned QUEUE_SIZE = 1024*1024;
static const unsigned NUM_WRITERS = 2;
static const unsigned PUSHES_PER_ITERATION = 2;

// Data to read/write using actions pumped through the queue
struct Record {
	Record() : read_count(0), write_count(0) {}

	AtomicInt read_count;
	AtomicInt write_count;
};

Record data[NUM_DATA];


// Actions pumped through the queue to manipulate data
struct WriteAction {
	WriteAction(unsigned idx)
		: index(idx)/*, has_read(false)*/ {}

	inline void read() const {
		//cout << "READ " << index << "\r\n";
		//assert(!has_read);
		++(data[index].read_count);
		//has_read = true;
	};

	unsigned index;
	//bool has_read;
};


// The victim
SRMWQueue<WriteAction> queue(QUEUE_SIZE); 


class WriteThread : public Thread {
protected:
	void _run() {

		cout << "Writer starting.\r\n";

		// Wait for everything to get ready
		sleep(2);

		while (true) {
			for (unsigned j=0; j < PUSHES_PER_ITERATION; ++j) {
				unsigned i = rand() % NUM_DATA;
				if (queue.push(WriteAction(i))) {
					++(data[i].write_count);
					//cout << "WRITE " << i << "\r\n";
				} else {
					cerr << "FAILED WRITE\r\n";
				}
			}

			// FIXME: remove!
			//if (rand() % 20)
			//	usleep(1);

			// This thread will never cancel without this here since
			// all the stuff about is cancellation point free
			// (good!  RT safe)
			pthread_testcancel();
		}

		cout << "Writer exiting." << endl;
	}
};


// Returns 0 if all read count/write count pairs are equal,
// otherwise how far off total count was
unsigned
data_is_sane()
{
	unsigned ret = 0;
	for (unsigned i=0; i < NUM_DATA; ++i) {
		unsigned diff = abs(data[i].read_count.get() - data[i].write_count.get());
		ret += diff;
	}

	return ret;
}


void
dump_data()
{
	for (unsigned i=0; i < NUM_DATA; ++i) {
		cout << i << ":\t" << data[i].read_count.get()
			<< "\t : \t" << data[i].write_count.get();
		if (data[i].read_count.get() == data[i].write_count.get())
			cout << "\t OK" << endl;
		else
			cout << "\t FAIL" << endl;
	}
}



int main()
{
	unsigned long total_processed = 0;
	
	cout << "Testing size" << endl;
	for (unsigned i=0; i < queue.capacity(); ++i) {
		queue.push(i);
		if (i == queue.capacity()-1) {
			if (!queue.full()) {
				cerr << "ERROR: Should be full at " << i
						<< " (size " << queue.capacity() << ")" << endl;
				return -1;
			}
		} else {
			if (queue.full()) {
				cerr << "ERROR: Prematurely full at " << i
					<< " (size " << queue.capacity() << ")" << endl;
				return -1;
			}
		}
	}
	
	for (unsigned i=0; i < queue.capacity(); ++i)
		queue.pop();

	if (!queue.empty()) {
		cerr << "ERROR: Should be empty" << endl;
		return -1;
	}
	
	cout << "Testing concurrent reading/writing" << endl;
	vector<WriteThread*> writers(NUM_WRITERS, new WriteThread());

	struct termios orig_term;
    struct termios raw_term;

    cfmakeraw(&raw_term);
    if (tcgetattr(0, &orig_term) != 0) return 1; //save terminal settings
    if (tcsetattr(0, TCSANOW, &raw_term) != 0) return 1; //set to raw
    fcntl(0, F_SETFL, O_NONBLOCK); //set to nonblocking IO on stdin


	for (unsigned i=0; i < NUM_WRITERS; ++i) {
		writers[i]->set_name(string("Writer ") + (char)('0' + i));
		writers[i]->start();
	}

	// Read
	while (getchar() == -1) {
		unsigned count = 0;
		while (count < queue.capacity() && !queue.empty()) {
			WriteAction action = queue.front();
			queue.pop();
			action.read();
			++count;
			++total_processed;
		}

		/*if (count > 0)
			cout << "Processed " << count << " requests\t\t"
				<< "(total " << total_processed << ")\r\n";*/
		
		//if (total_processed > 0 && total_processed % 128l == 0)
		//	cout << "Total processed: " << total_processed << "\r\n";
	}

    if (tcsetattr(0, TCSANOW, &orig_term) != 0) return 1; //restore

	cout << "Finishing." << endl;
	
	// Stop the writers
	for (unsigned i=0; i < NUM_WRITERS; ++i)
		writers[i]->stop();

	cout << "\n\n****************** DONE *********************\n\n";

	unsigned leftovers = 0;

	// Drain anything left in the queue
	while (!queue.empty()) {
		WriteAction action = queue.front();
		queue.pop();
		action.read();
		leftovers++;
		++total_processed;
	}

	if (leftovers > 0)
		cout << "Processed " << leftovers << " leftovers." << endl;


	cout << "\n\n*********************************************\n\n";

	cout << "Total processed: " << total_processed << endl;
	if (total_processed > INT_MAX)
		cout << "(Counter had to wrap)" << endl;
	else 
		cout << "(Counter did NOT have to wrap)" << endl;


	unsigned diff = data_is_sane();

	if (diff == 0) {
		cout << "PASS" << endl;
	} else {
		cout << "FAILED BY " << diff << endl;
	//	dump_data();
	}
	
	dump_data();

	return 0;
}


#if 0
int main()
{
	//SRSWQueue<int> q(10);
	SRMWQueue<int> q(10);

	cout << "New queue.  Should be empty: " << q.empty() << endl;
	cout << "Capacity: " << q.capacity() << endl;
	//cout << "Fill: " << q.fill() << endl;
	
	for (uint i=0; i < 5; ++i) {
		q.push(i);
		assert(!q.full());
		q.pop();
	}
	cout << "Pushed and popped 5 elements.  Queue should be empty: " << q.empty() << endl;
	//cout << "Fill: " << q.fill() << endl;

	for (uint i=10; i < 20; ++i) {
		assert(q.push(i));
	}
	cout << "Pushed 10 elements.  Queue should be full: " << q.full() << endl;
	//cout << "Fill: " << q.fill() << endl;

	cout << "The digits 10->19 should print: " << endl;
	while (!q.empty()) {
		int foo = q.front();
		q.pop();
		cout << "Popped: " << foo << endl;
	}
	cout << "Queue should be empty: " << q.empty() << endl;
	//cout << "Fill: " << q.fill() << endl;
	
	cout << "Attempting to add eleven elements to queue of size 10. Only first 10 should succeed:" << endl;
	for (uint i=20; i <= 39; ++i) {
		cout << i;
		//cout << " - Fill: " << q.fill();
		cout << " - full: " << q.full();
		cout << ", succeeded: " << q.push(i) << endl;
	}
	
	return 0;
}
#endif

