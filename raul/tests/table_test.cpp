#include <string>
#include <iostream>
#include <utility>
#include <raul/Table.hpp>
#include <raul/TableImpl.hpp>

using namespace Raul;
using namespace std;

int
main()
{
	Table<int, int> t;
	for (size_t i=0; i < 20; ++i) {
		int val = rand()%10;
		t.insert(make_pair(val, val));
	}

	t[20] = 20;
	t[21] = 21;

	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;

	Table<int, int>::iterator first = t.begin();
	++first;
	Table<int, int>::iterator last = first;
	++last; ++last; ++last;

	cout << "Erasing elements 1..3:" << endl;
	t.erase(first, last);

	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	cout << "Erasing elements 0..3" << endl;
	first = t.begin();
	last = first;
	++last; ++last; ++last;
	t.erase(first, last);

	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	cout << "Erasing elements end()-2..end()" << endl;
	last = t.end();
	first = last;
	--first; --first;
	t.erase(first, last);

	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	cout << "Erasing everything" << endl;
	first = t.begin();
	last = t.end();
	t.erase(first, last);

	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	/* **** */

	cout << "Assuming you built with debugging, if this continues to run "
		<< "and chews your CPU without dying, everything's good." << endl;
	srand(time(NULL));

	Table<string, string> st;

	st.insert(make_pair("apple", "core"));
	st.insert(make_pair("candy", "cane"));
	st.insert(make_pair("banana", "peel"));
	//st["alpha"] = "zero";
	//st["zeta"] = "one";

	st.erase("banana");
	
	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;

	while (true) {
		Table<int, int> t;

		size_t table_size = (rand() % 1000) + 1;

		for (size_t i=0; i < table_size; ++i) {
			int val = rand()%100;
			t.insert(make_pair(val, ((val + 3) * 17)));
		}

		for (int i=0; i < (int)table_size; ++i) {
			int val = rand()%100;
			Table<int, int>::iterator iter = t.find(val);
			assert(iter == t.end() || iter->second == (val + 3) * 17);
		}

		/*cout << "CONTENTS:" << endl;

		  for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i) {
		  cout << i->first << ": " << i->second << endl;
		  }

		  Table<int,int>::iterator i = t.find(7);
		  if (i != t.end())
		  cout << "Find: 7: " << i->second << endl;
		  */
	}

	return 0;
}

