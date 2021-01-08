/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "xwayland_clipboard.h"

#include "mir/scene/clipboard.h"

#include <xcb/xfixes.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace
{
auto create_selection_window(mf::XCBConnection const& connection) -> xcb_window_t
{
    uint32_t const attrib_values[]{XCB_EVENT_MASK_PROPERTY_CHANGE};

    xcb_window_t const selection_window = xcb_generate_id(connection);
    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        selection_window,
        connection.root_window(),
        0, 0, 10, 10, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection.screen()->root_visual,
        XCB_CW_EVENT_MASK, attrib_values);

    xcb_set_selection_owner(connection, selection_window, connection.CLIPBOARD_MANAGER, XCB_TIME_CURRENT_TIME);

    uint32_t const mask =
        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
    xcb_xfixes_select_selection_input(connection, selection_window, connection.CLIPBOARD, mask);

    return selection_window;
}
}

class mf::XWaylandClipboard::ClipboardObserver : public scene::ClipboardObserver
{
public:
    ClipboardObserver(XWaylandClipboard* const owner)
        : owner{owner}
    {
    }

    void paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source) override
    {
        owner->paste_source_set(source);
    }

private:
    XWaylandClipboard* const owner;
};

mf::XWaylandClipboard::XWaylandClipboard(
    std::shared_ptr<XCBConnection> const& connection,
    std::shared_ptr<scene::Clipboard> const& clipboard)
    : connection{connection},
      clipboard{clipboard},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)},
      selection_window{create_selection_window(*connection)}
{
    clipboard->register_interest(clipboard_observer);
    if (auto const source = clipboard->paste_source())
    {
        paste_source_set(source);
    }
}

mf::XWaylandClipboard::~XWaylandClipboard()
{
    clipboard->unregister_interest(*clipboard_observer);
    xcb_destroy_window(*connection, selection_window);
    connection->flush();
}

void mf::XWaylandClipboard::paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source)
{
    (void)source;
    // TODO
}
