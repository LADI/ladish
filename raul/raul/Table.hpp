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

#ifndef RAUL_TABLE_HPP
#define RAUL_TABLE_HPP

#include <vector>
#include <algorithm>

namespace Raul {


/** Slow insertion, fast lookup, cache optimized, super fast sorted iteration.
 *
 * This has the advantage over std::map that iterating over the collection
 * is super fast, and iteration is sorted.  Probably also faster due to all
 * data being in a single chunk of memory, cache optimized, etc.
 */
template <typename K, typename T>
class Table {
public:
	Table<K, T>() : _size(0), _array(NULL) {}
	~Table<K, T>() { free(_array); }
	
	void clear() { free(_array); _array = NULL; _size = 0; }

	typedef std::pair<K,T> Entry;

	struct const_iterator {
		const_iterator(const Table<K,T>& t, size_t i) : _table(t), _index(i) {}
		inline const Entry& operator*() const { return _table._array[_index]; }
		inline const Entry* operator->() const { return &_table._array[_index]; }
		inline const_iterator& operator++() { ++_index; return *this; }
		inline bool operator==(const const_iterator& i) const { return _index == i._index; }
		inline bool operator!=(const const_iterator& i) const { return _index != i._index; }
	private:
		const Table<K,T>& _table;
		size_t _index;
	};
	
	struct iterator {
		iterator(Table<K,T>& t, size_t i) : _table(t), _index(i) {}
		inline Entry& operator*() const { return _table._array[_index]; }
		inline Entry* operator->() const { return &_table._array[_index]; }
		inline iterator& operator++() { ++_index; return *this; }
		inline bool operator==(const iterator& i) const { return _index == i._index; }
		inline bool operator!=(const iterator& i) const { return _index != i._index; }
		operator const_iterator() { return *(const_iterator*)this; }
	private:
		Table<K,T>& _table;
		size_t _index;
	};

	inline size_t size() const { return _size; }

	void insert(K key, T value);
	void remove(K& key);
	void remove(iterator i);
	
	const_iterator find(const K& key) const;
	iterator find(const K& key);

	const_iterator begin() const { return const_iterator(*this, 0); }
	const_iterator end()   const { return const_iterator(*this, size()); }
	iterator       begin()       { return iterator(*this, 0); }
	iterator       end()         { return iterator(*this, size()); }

private:
#ifndef NDEBUG
	bool is_sorted() const;
#endif

	friend class iterator;
	

	size_t  _size;
	Entry*  _array;
};


} // namespace Raul

#endif // RAUL_TABLE_HPP
