/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/server/compositor/bypass.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/fake_renderable.h"

#include <gtest/gtest.h>
#include <memory>

namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mtd=mir::test::doubles;

struct BypassMatchTest : public testing::Test
{
    geom::Rectangle const primary_monitor{{0, 0},{1920, 1200}};
    geom::Rectangle const secondary_monitor{{1920, 0},{1920, 1200}};
};

TEST_F(BypassMatchTest, nothing_matches_nothing)
{
    mg::RenderableList empty_list{};
    mc::BypassMatch matcher(primary_monitor);

    EXPECT_EQ(empty_list.end(), std::find_if(empty_list.begin(), empty_list.end(), matcher));
}

TEST_F(BypassMatchTest, small_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, single_fullscreen_window_bypassed)
{
    auto window = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{window};

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(window, *it);
}

TEST_F(BypassMatchTest, translucent_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.5f)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, hidden_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, unposted_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, true, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, shaped_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, offset_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(10, 50, 1920, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, obscured_fullscreen_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, translucently_obscured_fullscreen_window_not_bypassed)
{   // Regression test for LP: #1266385
    mc::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50, 0.5f)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, unobscured_fullscreen_window_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, unobscured_fullscreen_alpha_window_not_bypassed)
{
    mc::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.9f)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, many_fullscreen_windows_only_bypass_top)
{
    mc::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(9, 10, 11, 12),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(5, 6, 7, 8),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, many_fullscreen_windows_only_bypass_top_rectangular)
{
    mc::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.5f, false),
        std::make_shared<mtd::FakeRenderable>(9, 10, 11, 12),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true),
        std::make_shared<mtd::FakeRenderable>(5, 6, 7, 8),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false),
        bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, nonrectangular_not_bypassable)
{
    mc::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, nonvisible_not_bypassble)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, false, true)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, offscreen_not_bypassable)
{
    mc::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, true, false)
    };
    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassMatchTest, multimonitor_one_bypassed)
{
    mc::BypassMatch primary_matcher(primary_monitor);
    mc::BypassMatch secondary_matcher(secondary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        bypassed
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), primary_matcher));

    auto it = std::find_if(list.begin(), list.end(), secondary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, dual_bypass)
{
    mc::BypassMatch primary_matcher(primary_monitor);
    mc::BypassMatch secondary_matcher(secondary_monitor);

    auto primary_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto secondary_bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        primary_bypassed,
        secondary_bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), primary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(primary_bypassed, *it);

    it = std::find_if(list.begin(), list.end(), secondary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(secondary_bypassed, *it);
}

TEST_F(BypassMatchTest, multimonitor_oversized_no_bypass)
{
    mc::BypassMatch primary_matcher(primary_monitor);
    mc::BypassMatch secondary_matcher(secondary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 3840, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), primary_matcher));
    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), secondary_matcher));
}
