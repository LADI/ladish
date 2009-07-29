/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *
 **************************************************************************
 * This file contains implementation of the Widget class
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PATCHAGE_WIDGET_HPP
#define PATCHAGE_WIDGET_HPP

#include <libglademm/xml.h>
#include <boost/utility.hpp>

template <typename W>
class Widget : public boost::noncopyable {
public:
  Widget(Glib::RefPtr<Gnome::Glade::Xml> xml, const std::string& name) {
    init(xml, name);
  }

  Widget()
  {
    _me = 0;
  }

  void init(Glib::RefPtr<Gnome::Glade::Xml> xml, const std::string& name)
  {
    xml->get_widget(name.c_str(), _me);
  }

  void destroy() { delete _me; }

  W*       get()              { return _me; }
  const W* get() const        { return _me; }
  W*       operator->()       { return _me; }
  const W* operator->() const { return _me; }
  W&       operator*()        { return *_me; }
  const W& operator*() const  { return *_me; }

private:
  W* _me;
};
  

#endif // PATCHAGE_WIDGET_HPP
