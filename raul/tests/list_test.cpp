#include <iostream>
#include <cstddef>
#include <raul/List.hpp>

using namespace std;
using namespace Raul;


int main()
{
	List<int> l;

	l.push_back(new ListNode<int>(1));
	l.push_back(new ListNode<int>(2));
	l.push_back(new ListNode<int>(3));
	l.push_back(new ListNode<int>(4));
	l.push_back(new ListNode<int>(5));
	l.push_back(new ListNode<int>(6));
	l.push_back(new ListNode<int>(7));
	l.push_back(new ListNode<int>(8));

	cout << "List:" << endl;
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;


	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 4) {
			l.erase(i);
			break;
		}
	}

	cout << "Removed 4 (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	/*l.remove(1);

	cout << "Removed 1 (head) (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	*/

	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 2) {
			l.erase(i);
			break;
		}
	}

	cout << "Removed 2 (head) (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	/*l.remove(5);

	cout << "Removed 5 (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	l.remove(8);

	cout << "Removed 8 (tail) (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	*/
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 7) {
			l.erase(i);
			break;
		}
	}

	cout << "Removed 7 (tail) (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;

	List<int> r;
	r.push_back(new ListNode<int>(9));
	r.erase(r.begin());
	cout << "Should not see ANY numbers:\n";
	for (List<int>::iterator i = r.begin(); i != r.end(); ++i) {
		cout << *i << endl;
	}
	
	cout << "\n\nTesting appending to an empty list:\n";
	l.clear();

	List<int> l2;
	l2.push_back(new ListNode<int>(1));
	l2.push_back(new ListNode<int>(2));
	l2.push_back(new ListNode<int>(3));
	l2.push_back(new ListNode<int>(4));
	
	cout << "l1:\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	
	cout << "l2:\n";
	for (List<int>::iterator i = l2.begin(); i != l2.end(); ++i) {
		cout << *i << endl;
	}

	l.append(l2);
	cout << "l1.append(l2):\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	
	cout << "\n\nAppending non-empty lists:\n";
	l2.push_back(new ListNode<int>(5));
	l2.push_back(new ListNode<int>(6));
	l2.push_back(new ListNode<int>(7));
	l2.push_back(new ListNode<int>(8));
	
	cout << "l1:\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	
	cout << "l2:\n";
	for (List<int>::iterator i = l2.begin(); i != l2.end(); ++i) {
		cout << *i << endl;
	}

	l.append(l2);
	cout << "l1.append(l2):\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	
	
	cout << "\n\nAppending an empty list:\n";
	
	cout << "l1:\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	
	cout << "l2:\n";
	for (List<int>::iterator i = l2.begin(); i != l2.end(); ++i) {
		cout << *i << endl;
	}

	l.append(l2);
	cout << "l1.append(l2):\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}

	return 0;
}
