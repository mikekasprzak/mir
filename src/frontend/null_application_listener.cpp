/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/application_listener.h"

void mir::frontend::NullApplicationListener::application_connect_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_create_surface_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_next_buffer_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_release_surface_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_disconnect_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_drm_auth_magic_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_configure_surface_called(std::string const&)
{
}

void mir::frontend::NullApplicationListener::application_error(
        std::string const&,
        char const* ,
        std::string const& )
{
}
