#include <iostream>
#include <raul/Path.hpp>

using namespace std;
using namespace Raul;

int
main()
{
	cerr << "1's are good..." << endl << endl;

	cerr <<  (Path("/").is_parent_of(Path("/foo"))) << endl;
	cerr <<  (Path("/foo").is_parent_of(Path("/foo/bar"))) << endl;
	cerr << !(Path("/foo").is_parent_of(Path("/foo2"))) << endl;
	
	cerr << Path::nameify("Signal Level [dB]") << endl;

    cerr << endl << endl << "Descendants..." << endl;
    cerr << "/     /foo     " << Path::descendant_comparator("/", "/foo") << endl;
    cerr << "/foo  /foo/bar " <<  Path::descendant_comparator("/foo", "/foo/bar") << endl;
    cerr << "/foo  /foo     " << Path::descendant_comparator("/foo", "/foo") << endl;
    cerr << "/     /        " << Path::descendant_comparator("/", "/") << endl;
    cerr << "/baz  /        " << Path::descendant_comparator("/baz", "/") << endl;

	return 0;
}
