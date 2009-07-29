/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the lash_client class
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

#include "common.h"
#include "lash_client.hpp"
#include "Patchage.hpp"
//#include "globals.hpp"

struct lash_client_impl
{
        lash_proxy * lash_ptr;
        project * project_ptr;
        string id;
        string name;
};

lash_client::lash_client(
        lash_proxy * lash_ptr,
        project * project_ptr,
        const string& id,
        const string& name)
{
        _impl_ptr = new lash_client_impl;
        _impl_ptr->lash_ptr = lash_ptr;
        _impl_ptr->project_ptr = project_ptr;
        _impl_ptr->id = id;
        _impl_ptr->name = name;

        //g_app->info_msg("client created");
}

lash_client::~lash_client()
{
        delete _impl_ptr;
        //g_app->info_msg("client destroyed");
}

project *
lash_client::get_project()
{
        return _impl_ptr->project_ptr;
}

void
lash_client::get_id(
        string& id)
{
        id = _impl_ptr->id;
}

void
lash_client::get_name(
        string& name)
{
        name = _impl_ptr->name;
}

void
lash_client::do_rename(
        const string& name)
{
        if (_impl_ptr->name != name)
        {
                //_impl_ptr->lash_ptr->client_rename(_impl_ptr->id, name);
        }
}

void
lash_client::on_name_changed(
        const string& name)
{
        _impl_ptr->name = name;
        _signal_renamed.emit();
}
