#include <string>
#include <iostream>
#include <utility>
#include <map>
#include <set>
#include <tr1/unordered_map>
#include <sys/time.h>
#include <raul/PathTable.hpp>
#include <raul/Table.hpp>
#include <raul/TableImpl.hpp>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION 1
#include <boost/functional/hash.hpp>

using namespace Raul;
using namespace std;

int range_end_val;

bool range_comparator(const int& a, const int& b)
{
	bool ret = (b >= a && b <= range_end_val);
	//cout << "COMP: " << a << " . " << b << " = " << ret << endl;
	return ret;
}

void benchmark(size_t n);

int
main(int argc, char** argv)
{
	if (argc == 3 && !strcmp(argv[1], "-b")) {
		benchmark(atoi(argv[2]));
		return 0;
	}

	cout << "run with -b num_elems to benchmark" << endl;
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

	Table<Path,char> yanked = pt.yank(quux, quux_end);
	
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

string
random_string()
{
	string ret(60, 'A' + (rand() % 26));
	return ret;
}


void
benchmark(size_t n)
{
	cout << "Benchmarking with n = " << n << endl;

	int useless_accumulator = 0;

	srand(time(NULL));

	vector<string> values(n);
	for (size_t i=0; i < n; ++i)
		values.push_back(random_string());

	timeval t1;
	t1.tv_sec=0;
	t1.tv_usec=0;
	
	timeval t2;
	t2.tv_sec=0;
	t2.tv_usec=0;
	

	/** std::map **/

	std::map<string,int> m;

	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		m.insert(make_pair(values[i], i));
	
	gettimeofday(&t2, NULL);

	float delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::map time to insert " << n << " values: \t" << delta_t << endl;
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		useless_accumulator += m.find(values[i])->second;
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::map time to lookup " << n << " values: \t" << delta_t << endl;
	
	
	/** std::set **/

	std::set<std::string> s;

	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		s.insert(values[i]);
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::set time to insert " << n << " values: \t" << delta_t << endl;
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		useless_accumulator += (int)(*s.find(values[i]))[0];
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::set time to lookup " << n << " values: \t" << delta_t << endl;
	
	
	/** sorted std::vector **/

	/*std::vector<int> v;

	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		v.push_back(values[i]);

	sort(v.begin(), v.end());
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::vector (sorted) time to insert " << n << " values: \t" << delta_t << endl;
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		useless_accumulator += *lower_bound(v.begin(), v.end(), values[i]);
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "std::vector (sorted) time to lookup " << n << " values: \t" << delta_t << endl;*/


	/** Raul::Table **/
	
	Raul::Table<string,int> t(n);
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		t.insert(make_pair(values[i], i));
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "Raul::Table time to insert " << n << " values: " << delta_t << endl;
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		useless_accumulator += t.find(values[i])->second;
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "Raul::Table time to lookup " << n << " values: \t" << delta_t << endl;
	
	
	/** boost::hash && std::unordered_map **/
	
	tr1::unordered_map<string, int, boost::hash<string> > um;

	gettimeofday(&t1, NULL);
	
	um.rehash(n);
	
	for (size_t i=0; i < n; ++i)
		um.insert(make_pair(values[i], i));
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "tr1::unordered_map + boost::hash time to insert " << n << " values: \t" << delta_t << endl;
	
	gettimeofday(&t1, NULL);
	
	for (size_t i=0; i < n; ++i)
		useless_accumulator += um.find(values[i])->second;
	
	gettimeofday(&t2, NULL);

	delta_t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 0.000001f;

	cout << "tr1::unordered_map + boost::hash time to lookup " << n << " values: \t" << delta_t << endl;
}

