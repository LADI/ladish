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

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <raul/Table.hpp>

namespace Raul {

#ifndef NDEBUG
template <typename K, typename T>
bool
Table<K,T>::is_sorted() const
{
	if (_size <= 1)
		return true;
	
	K prev_key = _array[0].first;

	for (size_t i=1; i < _size; ++i)
		if (_array[i].first < prev_key)
			return false;
		else
			prev_key = _array[i].first;
	
	return true;
}
#endif


/** Binary search (O(log(n))) */
template <typename K, typename T>
typename Table<K,T>::iterator
Table<K,T>::find(const K& key)
{
	if (_size == 0)
		return end();

	size_t lower = 0;
	size_t upper = _size - 1;
	size_t i;
	
	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);
		Entry& elem = _array[i];

		if (elem.first == key)
			return iterator(*this, i);
		else if (i < _size-1 && elem.first < key)
			lower = i + 1;
		else if (i > 0)
			upper = i - 1;
		else
			break;
	}
	
	return end();
}


template <typename K, typename T>
void
Table<K,T>::insert(K key, T value)
{
	Entry new_entry = make_pair(key, value);

	if (_size == 0) {
		_array = (std::pair<K, T>*)malloc(sizeof(Entry) * 2);
		_array[0] = new_entry;
		++_size;
		return;
	} else if (_size == 1) {
		if (key > _array[0].first) {
			_array[1] = new_entry;
		} else {
			_array[1] = _array[0];
			_array[0] = new_entry;
		}
		++_size;
		return;
	}

	size_t lower = 0;
	size_t upper = _size - 1;
	size_t i;
	
	// Find the earliest element > key
	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);
		assert(i >= lower);
		assert(i <= upper);
		assert(_array[lower].first <= _array[i].first);
		assert(_array[i].first <= _array[upper].first);

		assert(i < _size);
		Entry& elem = _array[i];

		if (elem.first == key) {
			break;
		} else if (elem.first > key) {
			if (i == 0 || _array[i-1].first < key)
				break;
			upper = i - 1;
		} else {
			lower = i + 1;
		}
	}

	// Lil' off by one touchup :)
	if (i < _size && _array[i].first <= key)
		++i;
	
	_array = (Entry*)realloc(_array, (_size + 1) * sizeof(Entry));
	
	// FIXME: This could be faster using memmove etc...
	for (size_t j=_size; j > i; --j)
		_array[j] = _array[j-1];

	_array[i] = new_entry;

	++_size;

	assert(is_sorted());
}


template <typename K, typename T>
void
Table<K,T>::remove(K& key)
{
	iterator i = find(key);
	if (i != _array.end())
		remove(i);
}


template <typename K, typename T>
void
Table<K,T>::remove(iterator i)
{
	_array.erase(i);

#ifndef NDEBUG
	assert(_array.is_sorted());
#endif
}


} // namespace Raul

