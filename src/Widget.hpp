/* This file is part of Patchage
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Patchage is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Patchage is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Patchage.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WIDGET_HPP
#define WIDGET_HPP

#include <string>
#include <boost/utility.hpp>

template <typename W>
class Widget : public boost::noncopyable {
public:
	Widget(Glib::RefPtr<Gnome::Glade::Xml> xml, const std::string& name) {
		xml->get_widget(name.c_str(), _me);
	}
	
	W*       get()              { return _me; }
	const W* get() const        { return _me; }
	W*       operator->()       { return _me; }
	const W* operator->() const { return _me; }
	W&       operator*()        { return *_me; }
	const W& operator*() const  { return *_me; }

private:
	W* _me;
};

#endif // WIDGET_HPP
