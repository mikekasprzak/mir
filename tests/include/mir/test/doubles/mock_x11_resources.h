/*
 * Copyright © 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_
#define MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_

#include "src/platforms/x11/x11_resources.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockXCBConnection : public mir::X::XCBConnection
{
public:
    MOCK_CONST_METHOD0(has_error, int());
    MOCK_CONST_METHOD0(poll_for_event, xcb_generic_event_t*());
    MOCK_CONST_METHOD0(screen, xcb_screen_t*());
    MOCK_CONST_METHOD1(intern_atom, xcb_atom_t(std::string const& name));
    MOCK_CONST_METHOD0(generate_id, uint32_t());
    MOCK_CONST_METHOD5(create_window, void(
        xcb_window_t window,
        int16_t x, int16_t y,
        uint32_t value_mask,
        const void* value_list));
    MOCK_CONST_METHOD6(change_property, void(
        xcb_window_t window,
        xcb_atom_t property_atom,
        xcb_atom_t type_atom,
        uint8_t format,
        size_t length,
        void const* data));
    MOCK_CONST_METHOD1(map_window, void(xcb_window_t window));
    MOCK_CONST_METHOD1(destroy_window, void(xcb_window_t window));
    MOCK_CONST_METHOD0(flush, void());
    MOCK_CONST_METHOD0(connection, xcb_connection_t*());
};

class MockX11Resources : public mir::X::X11Resources
{
public:
    MockX11Resources()
        : X11Resources{std::make_unique<testing::NiceMock<MockXCBConnection>>(), nullptr}
    {
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_
