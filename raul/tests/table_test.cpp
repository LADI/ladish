#include <string>
#include <iostream>
#include <utility>
#include <raul/PathTable.hpp>
#include <raul/Table.hpp>
#include <raul/TableImpl.hpp>

using namespace Raul;
using namespace std;

int range_end_val;

bool range_comparator(const int& a, const int& b)
{
	bool ret = (b >= a && b <= range_end_val);
	//cout << "COMP: " << a << " . " << b << " = " << ret << endl;
	return ret;
}


int
main()
{
	srand(time(NULL));

	range_end_val = rand()%10;

	Table<int, int> t;
	for (size_t i=0; i < 20; ++i) {
		int val = rand()%10;
		t.insert(make_pair(val, val));
	}

	t[20] = 20;
	t[21] = 21;
	
	cout << "Contents:" << endl;
	for (Table<int,int>::const_iterator i = t.begin(); i != t.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	std::cout << "Range " << t.begin()->first << " .. " << range_end_val << std::endl;

	Table<int,int>::const_iterator range_begin = t.begin();
	++range_begin; ++range_begin;
	
	Table<int,int>::iterator range_end = t.find_range_end(t.begin(), range_comparator);

	for (Table<int,int>::const_iterator i = t.begin(); i != range_end; ++i)
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

	PathTable<char> pt;
	pt.insert(make_pair("/foo", 'a'));
	pt.insert(make_pair("/bar", 'a'));
	pt.insert(make_pair("/bar/baz", 'b'));
	pt.insert(make_pair("/bar/bazz/NO", 'c'));
	pt.insert(make_pair("/bar/baz/YEEEAH", 'c'));
	pt.insert(make_pair("/bar/baz/YEEEAH/BOOOEEEEE", 'c'));
	pt.insert(make_pair("/bar/buzz", 'b'));
	pt.insert(make_pair("/bar/buzz/WHAT", 'c'));
	pt.insert(make_pair("/bar/buzz/WHHHhhhhhAT", 'c'));
	pt.insert(make_pair("/quux", 'a'));
	
	cout << "Paths: " << endl;
	for (PathTable<char>::const_iterator i = pt.begin(); i != pt.end(); ++i)
		cout << i->first << " ";
	cout << endl;
	
	PathTable<char>::const_iterator descendants_begin = pt.begin();
	size_t begin_index = rand() % pt.size();
	for (size_t i=0; i < begin_index; ++i)
		++descendants_begin;

	cout << "\nDescendants of " << descendants_begin->first << endl;
	PathTable<char>::const_iterator descendants_end = pt.find_descendants_end(descendants_begin);

	for (PathTable<char>::const_iterator i = pt.begin(); i != descendants_end; ++i)
		cout << i->first << " ";
	cout << endl;

	const Path yank_path("/bar");
	PathTable<char>::iterator quux = pt.find(yank_path);
	assert(quux != pt.end());
	PathTable<char>::iterator quux_end = pt.find_descendants_end(quux );
	assert(quux_end != quux);

	std::vector<std::pair<Path,char> > yanked = pt.yank(quux, quux_end);
	
	cout << "Yanked " << yank_path << endl;
	for (PathTable<char>::const_iterator i = pt.begin(); i != pt.end(); ++i)
		cout << i->first << " ";
	cout << endl;

	pt.cram(yanked);
	
	cout << "Crammed " << yank_path << endl;
	for (PathTable<char>::const_iterator i = pt.begin(); i != pt.end(); ++i)
		cout << i->first << " ";
	cout << endl;

	/* **** */

	cout << "\nAssuming you built with debugging, if this continues to run "
		<< "and chews your CPU without dying, everything's good." << endl;
	

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

