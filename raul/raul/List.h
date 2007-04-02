/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_LIST_H
#define RAUL_LIST_H

#include <cstddef>
#include <cassert>
#include "Deletable.h"
#include "AtomicPtr.h"
#include "AtomicInt.h"

namespace Raul {


/** A node in a List.
 *
 * This is exposed so the user can allocate ListNodes in different thread
 * than the list reader, and insert (e.g. via an Event) it later in the
 * reader thread.
 */
template <typename T>
class ListNode : public Raul::Deletable
{
public:
	ListNode(T elem) : _elem(elem) {}
	virtual ~ListNode() {}

	ListNode* next() const       { return _next.get(); }
	void      next(ListNode* ln) { _next = ln; }
	ListNode* prev() const       { return _prev.get(); }
	void      prev(ListNode* ln) { _prev = ln; }
	T&        elem()             { return _elem;}
	const T&  elem() const       { return _elem; }
	
private:
	T                   _elem;
	AtomicPtr<ListNode> _next;
	AtomicPtr<ListNode> _prev;
};



/** A realtime safe, (partially) thread safe doubly-linked list.
 * 
 * Elements can be added safely while another thread is reading the list.
 * Like a typical ringbuffer, this is single-reader single-writer threadsafe
 * only.  See documentation for specific functions for specifics.
 */
template <typename T>
class List : public Raul::Deletable
{
public:
	List() : _size(0), _end_iter(this), _const_end_iter(this)
	{
		_end_iter._listnode = NULL;
		_const_end_iter._listnode = NULL;
	}
	~List();

	void push_back(ListNode<T>* elem); // realtime safe
	void push_back(T& elem);           // NOT realtime safe

	void append(List<T>& list);

	void clear();

	/// Valid only in the write thread
	unsigned size() const { return (unsigned)_size.get(); }

	/// Valid for any thread
	bool empty() { return (_head.get() == NULL); }

	class iterator;

	/** Realtime safe const iterator for a List. */
	class const_iterator
	{
	public:
		const_iterator(const List<T>* const list);
		const_iterator(const iterator& i)
		: _list(i._list), _listnode(i._listnode) {}
	
		inline const T&        operator*();
		inline const_iterator& operator++();
		inline bool            operator!=(const const_iterator& iter) const;
		inline bool            operator!=(const iterator& iter) const;
		inline bool            operator==(const const_iterator& iter) const;
		inline bool            operator==(const iterator& iter) const;
	
		friend class List<T>;
		
	private:
		const List<T>* const _list;
		const ListNode<T>*   _listnode;
	};


	/** Realtime safe iterator for a List. */
	class iterator
	{
	public:
		iterator(List<T>* const list);

		inline T&        operator*();
		inline iterator& operator++();
		inline bool      operator!=(const iterator& iter) const;
		inline bool      operator!=(const const_iterator& iter) const;
		inline bool      operator==(const iterator& iter) const;
		inline bool      operator==(const const_iterator& iter) const;
	
		friend class List<T>;
		friend class List<T>::const_iterator;

	private:
		const List<T>* _list;
		ListNode<T>*   _listnode;
	};

	
	ListNode<T>* erase(const iterator iter);

	iterator find(const T& val);

	iterator begin();
	const iterator end() const;
	
	const_iterator begin() const;
	//const_iterator end()   const;

private:
	AtomicPtr<ListNode<T> > _head;
	AtomicPtr<ListNode<T> > _tail; ///< writer only
	AtomicInt               _size;
	iterator                _end_iter;
	const_iterator          _const_end_iter;
};




template <typename T>
List<T>::~List<T>()
{
	clear();
}


/** Clear the list, deleting all ListNodes contained (but NOT their contents!)
 *
 * Not realtime safe.
 */
template <typename T>
void
List<T>::clear()
{
	ListNode<T>* node = _head.get();
	ListNode<T>* next = NULL;

	while (node) {
		next = node->next();
		delete node;
		node = next;
	}

	_head = 0;
	_tail = 0;
	_size = 0;
}


/** Add an element to the list.
 *
 * Thread safe (may be called while another thread is reading the list).
 * Realtime safe.
 */
template <typename T>
void
List<T>::push_back(ListNode<T>* const ln)
{
	assert(ln);

	ln->next(NULL);
	
	if ( ! _head.get()) { // empty
		ln->prev(NULL);
		_tail = ln;
		_head = ln;
	} else {
		ln->prev(_tail.get());
		_tail.get()->next(ln);
		_tail = ln;
	}
	++_size;
}


/** Add an element to the list.
 *
 * Thread safe (may be called while another thread is reading the list).
 * NOT realtime safe (a ListNode is allocated).
 */
template <typename T>
void
List<T>::push_back(T& elem)
{
	ListNode<T>* const ln = new ListNode<T>(elem);

	assert(ln);

	ln->next(NULL);
	
	if ( ! _head.get()) { // empty
		ln->prev(NULL);
		_tail = ln;
		_head = ln;
	} else {
		ln->prev(_tail.get());
		_tail.get()->next(ln);
		_tail = ln;
	}
	++_size;
}


/** Append a list to this list.
 *
 * This operation is fast ( O(1) ).
 * The appended list is not safe to use concurrently with this call.
 *
 * The appended list will be empty after this call.
 *
 * Thread safe (may be called while another thread is reading the list).
 * Realtime safe.
 */
template <typename T>
void
List<T>::append(List<T>& list)
{
	ListNode<T>* const my_head    = _head.get();
	ListNode<T>* const my_tail    = _tail.get();
	ListNode<T>* const other_head = list._head.get();
	ListNode<T>* const other_tail = list._tail.get();

	// Appending to an empty list
	if (my_head == NULL && my_tail == NULL) {
		_head = other_head;
		_tail = other_tail;
		_size = list._size;
	} else if (other_head != NULL && other_tail != NULL) {
	
		other_head->prev(my_tail);

		// FIXME: atomicity an issue? _size < true size is probably fine...
		// no gurantee an iteration runs exactly size times though.  verify/document this.
		// assuming comment above that says tail is writer only, this is fine
		my_tail->next(other_head);
		_tail = other_tail;
		_size += list.size();
	}

	list._head = NULL;
	list._tail = NULL;
	list._size = 0;
}


/** Remove all elements equal to @val from the list.
 *
 * This function is realtime safe - it is the caller's responsibility to
 * delete the returned ListNode, or there will be a leak.
 */
#if 0
template <typename T>
ListNode<T>*
List<T>::remove(const T val)
{
	// FIXME: atomicity?
	ListNode<T>* n = _head;
	while (n) {
		if (n->elem() == elem)
			break;
		n = n->next();
	}
	if (n) {
		if (n == _head) _head = _head->next();
		if (n == _tail) _tail = _tail->prev();
		if (n->prev())
			n->prev()->next(n->next());
		if (n->next())
			n->next()->prev(n->prev());
		--_size;
		
		if (_size == 0)
			_head = _tail = NULL; // FIXME: Shouldn't be necessary
		
		return n;
	}
	return NULL;
}
#endif


/** Find an element in the list.
 *
 * This will only return the first element found.  If there are duplicated,
 * another call to find() will return the next, etc.
 */
template <typename T>
typename List<T>::iterator
List<T>::find(const T& val)
{
	for (iterator i = begin(); i != end(); ++i)
		if (*i == val)
			return i;

	return end();
}


/** Remove an element from the list using an iterator.
 * 
 * This function is realtime safe - it is the caller's responsibility to
 * delete the returned ListNode, or there will be a leak.
 * Thread safe (safe to call while another thread reads the list).
 * @a iter is invalid immediately following this call.
 */
template <typename T>
ListNode<T>*
List<T>::erase(const iterator iter)
{
	ListNode<T>* const n = iter._listnode;
	
	if (n) {
	
		ListNode<T>* const prev = n->prev();
		ListNode<T>* const next = n->next();

		// Removing the head (or the only element)
		if (n == _head.get())
			_head = next;

		// Removing the tail (or the only element)
		if (n == _tail.get())
			_tail = _tail.get()->prev();

		if (prev)
			n->prev()->next(next);

		if (next)
			n->next()->prev(prev);
	
		--_size;
	}

	return n;
}


//// Iterator stuff ////

template <typename T>
List<T>::iterator::iterator(List<T>* list)
: _list(list),
  _listnode(NULL)
{
}


template <typename T>
T&
List<T>::iterator::operator*()
{
	assert(_listnode);
	return _listnode->elem();
}


template <typename T>
inline typename List<T>::iterator&
List<T>::iterator::operator++()
{
	assert(_listnode);
	_listnode = _listnode->next();

	return *this;
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const const_iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::iterator::operator==(const iterator& iter) const
{
	return (_listnode == iter._listnode);
}


template <typename T>
inline bool
List<T>::iterator::operator==(const const_iterator& iter) const
{
	return (_listnode == iter._listnode);
}


template <typename T>
inline typename List<T>::iterator
List<T>::begin()
{
	typename List<T>::iterator iter(this);

	iter._listnode = _head.get();
	
	return iter;
}


template <typename T>
inline const typename List<T>::iterator
List<T>::end() const
{
	return _end_iter;
}



/// const_iterator stuff ///


template <typename T>
List<T>::const_iterator::const_iterator(const List<T>* const list)
: _list(list),
  _listnode(NULL)
{
}


template <typename T>
const T&
List<T>::const_iterator::operator*() 
{
	assert(_listnode);
	return _listnode->elem();
}


template <typename T>
inline typename List<T>::const_iterator&
List<T>::const_iterator::operator++()
{
	assert(_listnode);
	_listnode = _listnode->next();

	return *this;
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const const_iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::const_iterator::operator==(const const_iterator& iter) const
{
	return (_listnode == iter._listnode);
}


template <typename T>
inline bool
List<T>::const_iterator::operator==(const iterator& iter) const
{
	return (_listnode == iter._listnode);
}

template <typename T>
inline typename List<T>::const_iterator
List<T>::begin() const
{
	typename List<T>::const_iterator iter(this);
	iter._listnode = _head.get();
	return iter;
}

#if 0
template <typename T>
inline typename List<T>::const_iterator
List<T>::end() const
{
	/*typename List<T>::const_iterator iter(this);
	iter._listnode = NULL;
	iter._next = NULL;
	return iter;*/
	return _const_end_iter;
}
#endif


} // namespace Raul

#endif // RAUL_LIST_H
