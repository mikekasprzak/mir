/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display_configuration.h"
#include "mir/geometry/rectangle.h"
#include "hwc_display.h"

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

mga::HWCDisplay::HWCDisplay(const std::shared_ptr<AndroidFramebufferWindowQuery>& fb_window,
                            std::shared_ptr<HWCDevice> const& /*hwc_device*/)
 : AndroidDisplay(fb_window)
{
}

geom::Rectangle mga::HWCDisplay::view_area() const
{
    return AndroidDisplay::view_area();
}

void mga::HWCDisplay::clear()
{
}

bool mga::HWCDisplay::post_update()
{
    return true;
}

void mga::HWCDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& /*f*/)
{
}

std::shared_ptr<mg::DisplayConfiguration> mga::HWCDisplay::configuration()
{
    return std::shared_ptr<mg::DisplayConfiguration>();
}

void mga::HWCDisplay::make_current()
{
}
