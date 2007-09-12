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

//#define TABLE_SORT_DEBUG

namespace Raul {


/** Slow insertion, fast lookup, cache optimized, super fast sorted iteration.
 *
 * This has the advantage over std::map that iterating over the collection
 * is fast and sorted.  Possibly also faster in some situations due to all
 * data being in a single chunk of memory, cache optimized, etc.
 */
template <typename K, typename T>
class Table {
public:
	Table<K, T>() : _entries() {}
	Table<K, T>(size_t capacity) : _entries(capacity) {}
	
	void clear() { _entries.clear(); }
	bool empty() const { return _entries.empty(); }
	void reserve(size_t n) { _entries.reserve(n); }

	struct const_iterator {
		const_iterator(const Table<K,T>& t, size_t i) : _table(t), _index(i) {}
		inline const std::pair<const K, T> operator*() const { return _table._entries[_index]; }
		inline const std::pair<const K, T>* operator->() const { return (std::pair<const K, T>*)&_table._entries[_index]; }
		inline const_iterator& operator++() { ++_index; return *this; }
		inline const_iterator& operator--() { --_index; return *this; }
		inline bool operator==(const const_iterator& i) const { return _index == i._index; }
		inline bool operator!=(const const_iterator& i) const { return _index != i._index; }
		void operator=(const const_iterator& i) { _table = i._table; _index = i._index; }
	private:
		friend class Table<K,T>;
		const Table<K,T>& _table;
		size_t _index;
	};
	
	struct iterator {
		iterator(Table<K,T>& t, size_t i) : _table(t), _index(i) {}
		inline std::pair<K, T>& operator*() const { return (std::pair<K, T>&)_table._entries[_index]; }
		inline std::pair<K, T>* operator->() const { return (std::pair<K, T>*)&_table._entries[_index]; }
		inline iterator& operator++() { ++_index; return *this; }
		inline iterator& operator--() { --_index; return *this; }
		inline bool operator==(const iterator& i) const { return _index == i._index; }
		inline bool operator!=(const iterator& i) const { return _index != i._index; }
		operator const_iterator() { return *(const_iterator*)this; }
		iterator& operator=(const iterator& i) { _table = i._table; _index = i._index; return *this; }
	private:
		friend class Table<K,T>;
		Table<K,T>& _table;
		size_t _index;
	};

	inline size_t size() const { return _entries.size(); }

	std::pair<iterator,bool> insert(const std::pair<K, T>& entry);

	void erase(const K& key);
	void erase(iterator i);
	void erase(iterator start, iterator end);
	void erase_by_index(size_t start, size_t end);
	
	Table<K, T> yank(iterator start, iterator end);

	std::pair<iterator, bool> cram(const Table<K, T>& range);
	
	const_iterator find(const K& key) const;
	iterator find(const K& key);

	const_iterator find_range_end(const_iterator left, bool (*comp)(const K&,const K&)) const;
	iterator find_range_end(iterator left, bool (*comp)(const K&,const K&));

	T& operator[](const K& key);

	const_iterator begin() const { return const_iterator(*this, 0); }
	const_iterator end()   const { return const_iterator(*this, size()); }
	iterator       begin()       { return iterator(*this, 0); }
	iterator       end()         { return iterator(*this, size()); }

private:
#ifdef TABLE_SORT_DEBUG
	bool is_sorted() const;
#endif

	friend class iterator;
	
	typedef std::pair<K, T> Entry;

	std::vector<Entry> _entries;
};


} // namespace Raul

#endif // RAUL_TABLE_HPP
