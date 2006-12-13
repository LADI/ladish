#include <iostream>
#include <raul/Path.h>

using namespace std;

int
main()
{
	cerr << "1's are good..." << endl << endl;

	cerr <<  (Path("/").is_parent_of(Path("/foo"))) << endl;
	cerr <<  (Path("/foo").is_parent_of(Path("/foo/bar"))) << endl;
	cerr << !(Path("/foo").is_parent_of(Path("/foo2"))) << endl;
	
	return 0;
}
