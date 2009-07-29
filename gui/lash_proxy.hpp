/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to code that interfaces ladishd through D-Bus
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

#ifndef LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED
#define LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED

#include "Patchage.hpp"

struct lash_project_info
{
        std::string name;
        time_t modification_time;
        std::string description;
};

struct lash_loaded_project_properties
{
        bool modified_status;
        std::string description;
        std::string notes;
};

class session;

struct lash_proxy_impl;

class lash_proxy
{
public:
        lash_proxy(session * session_ptr);
        ~lash_proxy();

        bool is_active();
        void try_activate();
        void deactivate();
        void get_available_projects(std::list<lash_project_info>& projects);
        void load_project(const std::string& project_name);
        void save_all_projects();
        void save_project(const std::string& project_name);
        void close_project(const std::string& project_name);
        void close_all_projects();
        void project_rename(const std::string& old_name, const std::string& new_name);
        void get_loaded_project_properties(const std::string& name, lash_loaded_project_properties& properties);
        void project_set_description(const std::string& project_name, const std::string& description);
        void project_set_notes(const std::string& project_name, const std::string& notes);

private:
        lash_proxy_impl * _impl_ptr;
};

#endif // #ifndef LASH_PROXY_HPP__89E81B38_627F_41B9_AD08_DB119FB5F34C__INCLUDED
