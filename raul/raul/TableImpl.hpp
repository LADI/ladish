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

#ifndef RAUL_TABLE_IMPL_HPP
#define RAUL_TABLE_IMPL_HPP

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <raul/Table.hpp>

namespace Raul {

/* This is all a god awful mess.
 * Whoever decided you shouldn't be able to get an index from an
 * std::vector::iterator or vice-versa should be shot and pissed on.
 */

#ifdef TABLE_SORT_DEBUG
template <typename K, typename T>
bool
Table<K,T>::is_sorted() const
{
	if (size() <= 1)
		return true;
	
	K prev_key = _entries[0].first;

	for (size_t i=1; i < size(); ++i)
		if (_entries[i].first < prev_key)
			return false;
		else
			prev_key = _entries[i].first;
	
	return true;
}
#endif


/** Binary search (O(log(n))) */
template <typename K, typename T>
typename Table<K,T>::const_iterator
Table<K,T>::find(const K& key) const
{
	return ((Table<K,T>*)this)->find(key);
}


/** Binary search (O(log(n))) */
template <typename K, typename T>
typename Table<K,T>::iterator
Table<K,T>::find(const K& key)
{
	return find(begin(), end(), key);
}


/** Binary search (O(log(end - start))) */
template <typename K, typename T>
typename Table<K,T>::const_iterator
Table<K,T>::find(const_iterator start, const_iterator finish, const K& key) const
{
	return ((Table<K,T>*)this)->find(start, finish, key);
}


/** Binary search (O(log(end - start))) */
template <typename K, typename T>
typename Table<K,T>::iterator
Table<K,T>::find(const_iterator start, const_iterator finish, const K& key)
{
	if (size() == 0)
		return end();

	size_t lower = start._index;
	size_t upper = finish._index - 1;
	size_t i;
	
	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);
		const Entry& elem = _entries[i];

		if (elem.first == key)
			return iterator(*this, i);
		else if (i < size()-1 && elem.first < key)
			lower = i + 1;
		else if (i > 0)
			upper = i - 1;
		else
			break;
	}
	
	return end();
}


/** Find the end of a range using a custom comparator.
 * Two entries a, b are considered in the range if comp(a, b) returns true.
 *
 * Returns an iterator exactly one entry past the end of the range (possibly end()).
 *
 * WARNING: The restrictions on \a comparator are very strict:  ALL items
 * considered equal by \a comparator must be stored in the Table consecutively
 * i.e. there are no 3 values a, b, c S.T. comp(a) && ! comp(b) && comp(c).
 *
 * This is useful for very quickly finding all children of a Path, which
 * obey the above rule with lexicographical order.
 */
template <typename K, typename T>
typename Table<K,T>::const_iterator
Table<K,T>::find_range_end(const_iterator start, bool (*comp)(const K&,const K&)) const
{
	return ((Table<K, T>*)this)->find_range_end(*((iterator*)&start), comp);
}

	
/** Find the end of a range using a custom comparator.
 * Two entries a, b are considered in the range if comp(a, b) returns true.
 *
 * Returns an iterator exactly one entry past the end of the range (possibly end()).
 *
 * WARNING: The restrictions on \a comparator are very strict:  ALL items
 * considered equal by \a comparator must be stored in the Table consecutively
 * i.e. there are no 3 values a, b, c S.T. comp(a) && ! comp(b) && comp(c).
 *
 * This is useful for very quickly finding all children of a Path, which
 * obey the above rule with lexicographical order.
 */
template <typename K, typename T>
typename Table<K,T>::iterator
Table<K,T>::find_range_end(iterator start, bool (*comp)(const K&,const K&))
{
	if (size() == 0 || start == end())
		return this->end();

	const K& key = start->first;

	size_t lower = start._index;
	size_t upper = size() - 1;

	if (lower == upper)
		if (comp(key, _entries[lower].first))
			return iterator(*this, lower+1);
		else
			return this->end();

	size_t i;
	
	while (upper > lower) {
	
		i = lower + ((upper - lower) / 2);
		
		if (upper - lower == 1)
			if (comp(key, _entries[upper].first) && upper < size())
				return iterator(*this, upper+1);
			else if (lower < size())
				return iterator(*this, lower+1);

		const Entry& elem = _entries[i];

		// Hit
		if (comp(key, elem.first)) {
			
			if (i == size()-1 || !comp(key, _entries[i+1].first))
				return iterator(*this, i+1);
			else 
				lower = i;

		// Miss
		} else {

			upper = i;
			
		}
	}
	
	assert(comp(start->first, _entries[lower].first));
	assert(lower == size()-1 || !comp(start->first, _entries[lower+1].first));

	return iterator(*this, lower+1);
}
	

/** Erase and return a range of entries */
template <typename K, typename T>
SharedPtr< Table<K, T> >
Table<K, T>::yank(iterator start, iterator end)
{
	SharedPtr< Table<K, T> > ret(new Table<K, T>(end._index - start._index));
	for (size_t i=start._index; i < end._index; ++i)
		ret->_entries.at(i - start._index) = _entries[i];
	erase(start, end);
	return ret;
}


/** Cram a range of entries back in.
 * Range MUST follow the same ordering guidelines as find_range_end.
 * Return type is the same as insert, iterator points to first inserted entry */
template <typename K, typename T>
std::pair<typename Table<K,T>::iterator, bool>
Table<K, T>::cram(const Table<K,T>& range)
{
	/* FIXME: _way_ too slow */
	
	const size_t orig_size = size();

	if (range.size() == 0)
		return std::make_pair(end(), false);
	
	std::pair<iterator, bool> ret = insert(range._entries.front());
	if (range.size() == 1)
		return ret;
	
	const size_t insert_index = ret.first._index;

	std::vector<Entry> new_entries(orig_size + range.size());

	for (size_t i=0; i <= insert_index; ++i)
		new_entries.at(i) = _entries.at(i);

	for (size_t i=0; i < size() - insert_index; ++i)
		new_entries.at(new_entries.size() - 1 - i) = _entries.at(size() - 1 - i);
	
	for (size_t i=1; i < range.size(); ++i)
		new_entries.at(insert_index + i) = range._entries.at(i);
	
	_entries = new_entries;
	
	/*std::cerr << "********************************\n";
	for (size_t i=0; i < size(); ++i) {
		std::cerr << _entries[i].first << std::endl;
	}
	std::cerr << "********************************\n";*/
	
	assert(size() == orig_size + range.size());
#ifdef TABLE_SORT_DEBUG
	assert(is_sorted());
#endif

	return make_pair(iterator(*this, insert_index), true);
}


/** Add an item to the table, using \a entry.first as the search key.
 * An iterator to the element where value was set is returned, and a bool which
 * is true if an insertion took place, or false if an existing entry was updated.
 * Matches std::map::insert interface.
 * O(n) worst case
 * O(log(n)) best case (capacity is large enough)
 */
template <typename K, typename T>
std::pair<typename Table<K,T>::iterator, bool>
Table<K,T>::insert(const std::pair<K, T>& entry)
{
	const K& key = entry.first;
	const T& value = entry.second;
	
	if (size() == 0 || size() == 1 && key > _entries[0].first) {
		_entries.push_back(entry);
		return std::make_pair(iterator(*this, size()-1), true);
	} else if (size() == 1) {
		_entries.push_back(_entries[0]);
		_entries[0] = entry;
		return std::make_pair(begin(), true);
	}

	size_t lower = 0;
	size_t upper = size() - 1;
	size_t i;
	
	// Find the earliest element > key
	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);
		assert(i >= lower);
		assert(i <= upper);
		assert(_entries[lower].first <= _entries[i].first);
		assert(_entries[i].first <= _entries[upper].first);

		assert(i < size());
		Entry& elem = _entries[i];

		if (elem.first == key) {
			elem.second = value;
			return std::make_pair(iterator(*this, i), false);
		} else if (elem.first > key) {
			if (i == 0 || _entries[i-1].first < key)
				break;
			upper = i - 1;
		} else {
			lower = i + 1;
		}
	}

	// Lil' off by one touchup :)
	if (i < size() && _entries[i].first <= key)
		++i;
	
	_entries.push_back(_entries.back());

	// Shift everything beyond i right
	for (size_t j = size()-2; j > i; --j)
		_entries[j] = _entries[j-1];
	
	_entries[i] = entry;

#ifdef TABLE_SORT_DEBUG
	assert(is_sorted());
#endif
	
	return std::make_pair(iterator(*this, i), true);
}
	

/** Insert an item, and return a reference to it's value.
 *
 * This may be used to insert values with pretty syntax:
 *
 * table["gorilla"] = "killa";
 *
 * T must have a default constructor for this to be possible.
 */
template <typename K, typename T>
T&
Table<K, T>::operator[](const K& key)
{
	iterator i = find(key);
	if (i != end()) {
		return i->second;
	} else {
		std::pair<iterator,bool> ret = insert(make_pair(key, T()));
		return ret.first->second;
	}
}


template <typename K, typename T>
void
Table<K,T>::erase(const K& key)
{
	erase(find(key));
}


template <typename K, typename T>
void
Table<K,T>::erase(iterator i)
{
	if (i == end())
		return;

	const size_t index = i._index;

	// Shift left
	for (size_t j=index; j < size()-1; ++j)
		_entries[j] = _entries[j+1];

	_entries.pop_back();

#ifdef TABLE_SORT_DEBUG
	assert(is_sorted());
#endif
}


/** Erase a range of elements from \a first to \a last, including first but
 * not including last.
 */
template <typename K, typename T>
void
Table<K,T>::erase(iterator first, iterator last)
{
	const size_t first_index = first._index;
	const size_t last_index = last._index;

	Table<K,T>::erase_by_index(first_index, last_index);
}


/** Erase a range of elements from \a first_index to \a last_index, including
 * first_index but not including last_index.
 */
template <typename K, typename T>
void
Table<K,T>::erase_by_index(size_t first_index, size_t last_index)
{
	const size_t num_removed = last_index - first_index;

	// Shift left
	for (size_t j=first_index; j < size() - num_removed; ++j)
		_entries[j] = _entries[j + num_removed];

	_entries.resize(size() - num_removed);

#ifdef TABLE_SORT_DEBUG
	assert(is_sorted());
#endif
}


} // namespace Raul

#endif // RAUL_TABLE_IMLP_HPP

