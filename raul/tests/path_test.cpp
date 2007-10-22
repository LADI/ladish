#include <iostream>
#include <list>
#include <raul/Path.hpp>

using namespace std;
using namespace Raul;

int
main()
{
	list<string> names;
	names.push_back("foo+1bar(baz)");
	names.push_back("ThisCRAR");
	names.push_back("NAME");
	names.push_back("thing with a bunch of spaces");
	names.push_back("thing-with-a-bunch-of-dashes");
	names.push_back("CamelCaseABC");
	names.push_back("Signal Level [dB]");
	names.push_back("Gain dB");
	names.push_back("Dry/Wet Balance");
	names.push_back("Phaser1 - Similar to CSound's phaser1 by Sean Costello");

	cerr << "Nameification:" << endl;
	for (list<string>::iterator i = names.begin(); i != names.end(); ++i)
		cerr << *i << " -> " << Path::nameify(*i) << endl;

	cerr << "1's are good..." << endl << endl;

	cerr <<  (Path("/").is_parent_of(Path("/foo"))) << endl;
	cerr <<  (Path("/foo").is_parent_of(Path("/foo/bar"))) << endl;
	cerr << !(Path("/foo").is_parent_of(Path("/foo2"))) << endl;
	
    cerr << endl << endl << "Descendants..." << endl;
    cerr << "/     /foo     " << Path::descendant_comparator("/", "/foo") << endl;
    cerr << "/foo  /foo/bar " <<  Path::descendant_comparator("/foo", "/foo/bar") << endl;
    cerr << "/foo  /foo     " << Path::descendant_comparator("/foo", "/foo") << endl;
    cerr << "/     /        " << Path::descendant_comparator("/", "/") << endl;
    cerr << "/baz  /        " << Path::descendant_comparator("/baz", "/") << endl;

	return 0;
}
