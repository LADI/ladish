/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains declarations of global variables
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

#ifndef GLOBALS_HPP__08F1016E_CB85_4758_B5CD_92E0C15F5568__INCLUDED
#define GLOBALS_HPP__08F1016E_CB85_4758_B5CD_92E0C15F5568__INCLUDED

#if defined(PATCHAGE_WIDGET_HPP)
extern Glib::RefPtr<Gnome::Glade::Xml> g_xml;
#endif

#if defined(PATCHAGE_PATCHAGE_HPP)
extern Patchage * g_app;
#endif

#endif // #ifndef GLOBALS_HPP__08F1016E_CB85_4758_B5CD_92E0C15F5568__INCLUDED
