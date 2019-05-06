/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "layer_shell_v1.h"

#include "wl_surface.h"
#include "window_wl_surface_role.h"

#include "mir/shell/surface_specification.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

namespace
{

auto layer_shell_layer_to_mir_depth_layer(uint32_t layer) -> MirDepthLayer
{
    switch (layer)
    {
    case mw::LayerShellV1::Layer::background:
        return mir_depth_layer_background;
    case mw::LayerShellV1::Layer::bottom:
        return mir_depth_layer_below;
    case mw::LayerShellV1::Layer::top:
        return mir_depth_layer_above;
    case mw::LayerShellV1::Layer::overlay:
        return mir_depth_layer_overlay;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid Layer Shell layer " + std::to_string(layer)));
    }
}

}

namespace mir
{
namespace frontend
{

class LayerShellV1::Instance : wayland::LayerShellV1
{
public:
    Instance(wl_resource* new_resource, mf::LayerShellV1* shell)
        : LayerShellV1{new_resource, Version<1>()},
          shell{shell}
    {
    }

private:
    void get_layer_surface(
        wl_resource* new_layer_surface,
        wl_resource* surface,
        std::experimental::optional<wl_resource*> const& output,
        uint32_t layer,
        std::string const& namespace_) override;

    mf::LayerShellV1* const shell;
};

class LayerSurfaceV1 : public wayland::LayerSurfaceV1, public WindowWlSurfaceRole
{
public:
    LayerSurfaceV1(
        wl_resource* new_resource,
        WlSurface* surface,
        LayerShellV1 const& layer_shell,
        MirDepthLayer layer);

    ~LayerSurfaceV1() = default;

private:
    // from wayland::LayerSurfaceV1
    void set_size(uint32_t width, uint32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_exclusive_zone(int32_t zone) override;
    void set_margin(int32_t top, int32_t right, int32_t bottom, int32_t left) override;
    void set_keyboard_interactivity(uint32_t keyboard_interactivity) override;
    void get_popup(wl_resource* popup) override;
    void ack_configure(uint32_t serial) override;
    void destroy() override;

    // from WindowWlSurfaceRole
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(
        std::experimental::optional<geometry::Point> const& new_top_left,
        geometry::Size const& new_size) override;
};

}
}

// LayerShellV1

mf::LayerShellV1::LayerShellV1(
    struct wl_display* display,
    std::shared_ptr<Shell> const shell,
    WlSeat& seat,
    OutputManager* output_manager)
    : Global(display, Version<1>()),
      shell{shell},
      seat{seat},
      output_manager{output_manager}
{
}

void mf::LayerShellV1::bind(wl_resource* new_resource)
{
    new Instance{new_resource, this};
}

void mf::LayerShellV1::Instance::get_layer_surface(
    wl_resource* new_layer_surface,
    wl_resource* surface,
    std::experimental::optional<wl_resource*> const& output,
    uint32_t layer,
    std::string const& namespace_)
{
    (void)output; // TODO
    (void)namespace_; // TODO

    new LayerSurfaceV1(
        new_layer_surface,
        WlSurface::from(surface),
        *shell,
        layer_shell_layer_to_mir_depth_layer(layer));
}

// LayerSurfaceV1

mf::LayerSurfaceV1::LayerSurfaceV1(
    wl_resource* new_resource,
    WlSurface* surface,
    LayerShellV1 const& layer_shell,
    MirDepthLayer layer)
    : mw::LayerSurfaceV1(new_resource, Version<1>()),
      WindowWlSurfaceRole(
          &layer_shell.seat,
          wayland::LayerSurfaceV1::client,
          surface,
          layer_shell.shell,
          layer_shell.output_manager)
{
    shell::SurfaceSpecification spec;
    spec.state = mir_window_state_attached;
    spec.depth_layer = layer;
    apply_spec(spec);
    auto const serial = wl_display_next_serial(wl_client_get_display(wayland::LayerSurfaceV1::client));
    send_configure_event (serial, 0, 0);
}

void mf::LayerSurfaceV1::set_size(uint32_t width, uint32_t height)
{
    // HACK: Leaving zero sizes zero causes a validation error, and setting them to window_size() causes
    //       to not call WlSurfaceEventSink::handle_resize(). Setting them to 1 works for now. This will get fixed when
    //       we refactor size negotiation between the window manager and the frontend.
    if (width == 0)
        width = 1;
    if (height == 0)
        height = 1;
    WindowWlSurfaceRole::set_geometry(0, 0, width, height);
}

void mf::LayerSurfaceV1::set_anchor(uint32_t anchor)
{
    MirPlacementGravity edges = mir_placement_gravity_center;

    if (anchor & Anchor::top)
        edges = MirPlacementGravity(edges | mir_placement_gravity_north);

    if (anchor & Anchor::bottom)
        edges = MirPlacementGravity(edges | mir_placement_gravity_south);

    if (anchor & Anchor::left)
        edges = MirPlacementGravity(edges | mir_placement_gravity_west);

    if (anchor & Anchor::right)
        edges = MirPlacementGravity(edges | mir_placement_gravity_east);

    shell::SurfaceSpecification spec;
    spec.attached_edges = edges;
    apply_spec(spec);
}

void mf::LayerSurfaceV1::set_exclusive_zone(int32_t zone)
{
    (void)zone;
}

void mf::LayerSurfaceV1::set_margin(int32_t top, int32_t right, int32_t bottom, int32_t left)
{
    (void)top;
    (void)right;
    (void)bottom;
    (void)left;
}

void mf::LayerSurfaceV1::set_keyboard_interactivity(uint32_t keyboard_interactivity)
{
    (void)keyboard_interactivity;
}

void mf::LayerSurfaceV1::get_popup(struct wl_resource* popup)
{
    (void)popup;
}

void mf::LayerSurfaceV1::ack_configure(uint32_t serial)
{
    (void)serial;
}

void mf::LayerSurfaceV1::destroy()
{
    destroy_wayland_object();
}

void mf::LayerSurfaceV1::handle_resize(
    std::experimental::optional<geometry::Point> const& new_top_left,
    geometry::Size const& new_size)
{
    (void)new_top_left;

    auto const serial = wl_display_next_serial(wl_client_get_display(wayland::LayerSurfaceV1::client));
    send_configure_event (serial, new_size.width.as_int(), new_size.height.as_int());
}
