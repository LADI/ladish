// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED
#define LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED

#include "Patchage.hpp"

class lash_proxy
{
public:
	lash_proxy(Patchage* app);
	~lash_proxy();

	void get_loaded_projects(std::list<std::string>& projects);

private:
	void error_msg(const std::string& msg) const;
	void info_msg(const std::string& msg) const;

	static
	DBusHandlerResult
	dbus_message_hook(
		DBusConnection * connection,
		DBusMessage * message,
		void * proxy);

	bool
	call(
		bool response_expected,
		const char* iface,
		const char* method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		...);

	Patchage* _app;
	bool _server_responding;
};

#endif // #ifndef LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED
