/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/test_server.h>
#include "server_example_decoration.h"
#include "org_kde_kwin_server_decoration.h"

#include <miral/internal_client.h>
#include <miral/wayland_extensions.h>

#include <wayland-client.h>

#include <memory>
#include <mutex>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#if (WAYLAND_VERSION_MAJOR == 1) && (WAYLAND_VERSION_MINOR < 14)
#define MIR_NO_WAYLAND_FILTER
#endif

using namespace testing;

namespace
{
class WaylandClient
{
public:

    void operator()(struct wl_display* display)
    {
        code(display);
    }

    void operator()(std::weak_ptr<mir::scene::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        session_ = session;
    }

    std::shared_ptr<mir::scene::Session> session() const
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return session_.lock();
    }

    std::function<void (struct wl_display*)> code = [](auto){};

private:
    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;
};

struct WaylandExtensions : miral::TestServer
{
    WaylandExtensions()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    void run_as_client(std::function<void (struct wl_display*)>&& code)
    {
        bool client_run = false;
        std::condition_variable cv;
        std::mutex mutex;

        client.code = [&](struct wl_display* display)
            {
                std::lock_guard<decltype(mutex)> lock{mutex};
                code(display);
                client_run = true;
                cv.notify_one();
            };

        std::unique_lock<decltype(mutex)> lock{mutex};
        launcher.launch(client);
        cv.wait(lock, [&]{ return client_run; });
    }

    static auto constexpr unavailable_extension = "zxdg_shell_v6";

private:
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};

template<typename Type>
auto make_scoped(Type* owned, void(*deleter)(Type*)) -> std::unique_ptr<Type, void(*)(Type*)>
{
    return {owned, deleter};
}

void trivial_client(wl_display* display)
{
    auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
    wl_display_roundtrip(display);
}

struct ClientGlobalEnumerator
{
    using InterfaceNames = std::vector<std::string>;

    void operator()(wl_display* display)
    {
        auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
        wl_registry_add_listener(registry.get(), &registry_listener, interfaces.get());
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);
    }

    static void new_global(
        void* data,
        struct wl_registry* /*registry*/,
        uint32_t /*id*/,
        char const* interface,
        uint32_t /*version*/)
    {
        auto const interfaces = static_cast<InterfaceNames*>(data);
        interfaces->push_back(interface);
    }

    static void global_remove(
        void* /*data*/,
        struct wl_registry* /*registry*/,
        uint32_t /*name*/)
    {

    }

    static wl_registry_listener constexpr registry_listener = {
        new_global,
        global_remove
    };

    std::shared_ptr<InterfaceNames> interfaces = std::make_shared<InterfaceNames>();
};

wl_registry_listener constexpr ClientGlobalEnumerator::registry_listener;

struct ClientDecorationCreator
{
    ClientDecorationCreator(std::function<void()>&& test) :
        test{test} {}

    std::function<void()> test = []{};

    void operator()(wl_display* display)
    {
        auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
        wl_registry_add_listener(registry.get(), &registry_listener, this);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(decoration_manager, NotNull());
        ASSERT_THAT(shell, NotNull());

        auto const surface = make_scoped(
            wl_compositor_create_surface(compositor),
            &wl_surface_destroy);
        wl_display_roundtrip(display);

        auto const decoration = make_scoped(
            org_kde_kwin_server_decoration_manager_create(decoration_manager, surface.get()),
            &org_kde_kwin_server_decoration_release);
        wl_display_roundtrip(display);

        auto const window = make_scoped(wl_shell_get_shell_surface(shell, surface.get()), &wl_shell_surface_destroy);
        wl_shell_surface_set_toplevel(window.get());
        wl_display_roundtrip(display);

        wl_surface_commit(surface.get());
        wl_display_roundtrip(display);

        test();
    }

    static void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t /*version*/)
    {
        ClientDecorationCreator* self = static_cast<decltype(self)>(data);

        if (strcmp(interface, wl_compositor_interface.name) == 0)
        {
            self->compositor = static_cast<decltype(self->compositor)>
            (wl_registry_bind(registry, id, &wl_compositor_interface, 1));
        }

        if (strcmp(interface, org_kde_kwin_server_decoration_manager_interface.name) == 0)
        {
            self->decoration_manager = static_cast<decltype(self->decoration_manager)>
                (wl_registry_bind(registry, id, &org_kde_kwin_server_decoration_manager_interface, 1));
        }

        if (strcmp(interface, wl_shell_interface.name) == 0)
        {
            self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
        }
    }

    static void global_remove(
        void* /*data*/,
        struct wl_registry* /*registry*/,
        uint32_t /*name*/)
    {

    }

    static wl_registry_listener constexpr registry_listener = {
        new_global,
        global_remove
    };

    wl_compositor* compositor = nullptr;
    org_kde_kwin_server_decoration_manager* decoration_manager = nullptr;
    wl_shell* shell = nullptr;
};

wl_registry_listener constexpr ClientDecorationCreator::registry_listener;
}

TEST_F(WaylandExtensions, client_connects)
{
    bool client_connected = false;
    start_server();

    run_as_client([&](auto) { client_connected = true; });

    EXPECT_THAT(client_connected, Eq(true));
}


TEST_F(WaylandExtensions, filter_is_called)
{
    bool filter_called = false;

    miral::WaylandExtensions extensions;
    add_server_init(extensions);
    extensions.set_filter([&](auto, auto) { filter_called = true; return true; });
    start_server();

    run_as_client(trivial_client);

#ifndef MIR_NO_WAYLAND_FILTER
    EXPECT_THAT(filter_called, Eq(true));
#endif
}

TEST_F(WaylandExtensions, client_sees_default_extensions)
{
    ClientGlobalEnumerator enumerator_client;
    start_server();

    run_as_client(enumerator_client);

    auto const available_extensions = miral::WaylandExtensions::recommended();

    for (auto const& extension : available_extensions)
    {
        EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(extension)));
    }
}

TEST_F(WaylandExtensions, filter_controls_extensions_exposed_to_client)
{
    ClientGlobalEnumerator enumerator_client;
    miral::WaylandExtensions extensions;
    add_server_init(extensions);
    extensions.set_filter([&](auto, char const* protocol) { return strcmp(protocol, unavailable_extension); });
    start_server();

    run_as_client(enumerator_client);

#ifndef MIR_NO_WAYLAND_FILTER
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(std::string{unavailable_extension}))));
#endif
}

TEST_F(WaylandExtensions, server_can_add_bespoke_protocol)
{
    bool filter_saw_bespoke_extension = false;

    ClientGlobalEnumerator enumerator_client;
    miral::WaylandExtensions extensions;
    extensions.set_filter([&](auto, char const* protocol)
        { if (strcmp(protocol, unavailable_extension) == 0) filter_saw_bespoke_extension = true; return true; });
    extensions.add_extension(mir::examples::server_decoration_extension());
    add_server_init(extensions);

    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(mir::examples::server_decoration_extension().name)));
#ifndef MIR_NO_WAYLAND_FILTER
    EXPECT_THAT(filter_saw_bespoke_extension, Eq(true));
#endif
}

TEST_F(WaylandExtensions, add_extension_adds_protocol_to_supported_enabled_extensions)
{
    miral::WaylandExtensions extensions;

    EXPECT_THAT(extensions.all_supported(), Not(Contains(Eq(mir::examples::server_decoration_extension().name))));

    extensions.add_extension(mir::examples::server_decoration_extension());

    EXPECT_THAT(extensions.all_supported(), Contains(Eq(mir::examples::server_decoration_extension().name)));
}

TEST_F(WaylandExtensions, add_extension_disabled_by_default_adds_protocol_to_supported_extensions_only)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension_disabled_by_default(mir::examples::server_decoration_extension());
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(mir::examples::server_decoration_extension().name))));
}

TEST_F(WaylandExtensions, can_retrieve_application_for_client)
{
    miral::WaylandExtensions extensions;
    wl_client* client = nullptr;

    extensions.add_extension(mir::examples::server_decoration_extension([&](wl_client* c, auto){ client = c; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(client, NotNull());
            EXPECT_THAT(miral::application_for(client), NotNull());
        }});
}

TEST_F(WaylandExtensions, can_retrieve_application_for_surface)
{
    miral::WaylandExtensions extensions;

    wl_resource* surface = nullptr;
    extensions.add_extension(mir::examples::server_decoration_extension([&](auto, wl_resource* s){ surface =s; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(surface, NotNull());
            EXPECT_THAT(miral::application_for(surface), NotNull());
        }});
}


TEST_F(WaylandExtensions, can_retrieve_window_for_surface)
{
    miral::WaylandExtensions extensions;

    wl_resource* surface = nullptr;
    extensions.add_extension(mir::examples::server_decoration_extension([&](auto, wl_resource* s){ surface =s; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(surface, NotNull());
            EXPECT_THAT(miral::window_for(surface), Ne(nullptr));   // NotNull() fails to build on 16.04LTS
        }});
}

TEST_F(WaylandExtensions, some_extensions_are_supported)
{
    EXPECT_THAT(miral::WaylandExtensions::supported(), Not(IsEmpty()));
}

TEST_F(WaylandExtensions, some_extensions_are_recommended)
{
    EXPECT_THAT(miral::WaylandExtensions::recommended(), Not(IsEmpty()));
}

TEST_F(WaylandExtensions, recommeded_extensions_subset_of_supported_extensions)
{
    for (auto const& e : miral::WaylandExtensions::recommended())
    {
        EXPECT_THAT(miral::WaylandExtensions::supported(), Contains(e));
    }
}

TEST_F(WaylandExtensions, disable_can_remove_default_extensions)
{
    std::string const extension_to_remove{"zxdg_shell_v6"};
    miral::WaylandExtensions extensions;
    extensions.disable(extension_to_remove);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(extension_to_remove))));
}

TEST_F(WaylandExtensions, enable_can_enable_non_standard_extensions)
{
    std::string const extension_to_enable{"zwlr_layer_shell_v1"};
    miral::WaylandExtensions extensions;
    extensions.enable(extension_to_enable);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(extension_to_enable)));
}

TEST_F(WaylandExtensions, enable_can_enable_bespoke_extension)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension_disabled_by_default(mir::examples::server_decoration_extension());
    extensions.enable(mir::examples::server_decoration_extension().name);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(mir::examples::server_decoration_extension().name)));
}

TEST_F(WaylandExtensions, disable_can_disable_bespoke_extension)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension(mir::examples::server_decoration_extension());
    extensions.disable(mir::examples::server_decoration_extension().name);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(mir::examples::server_decoration_extension().name))));
}

TEST_F(WaylandExtensions, wayland_extensions_option_sets_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_WAYLAND_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("xdg_wm_base")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("wl_shell"))));
}

TEST_F(WaylandExtensions, add_extensions_option_adds_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_ADD_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("wl_shell")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("xdg_wm_base")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
}

TEST_F(WaylandExtensions, drop_extensions_option_removes_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_DROP_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("wl_shell")));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("xdg_wm_base"))));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("zwlr_layer_shell_v1"))));
}
