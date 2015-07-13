/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/compositor/timeout_queue.h"
#include "mir/test/doubles/stub_buffer.h"
#include <gtest/gtest.h>

using namespace testing;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

namespace
{
struct TimeoutQueue : Test
{
    TimeoutQueue()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{5};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    mc::TimeoutQueue schedule;
};
}

TEST_F(TimeoutQueue, throws_if_no_buffers)
{
    EXPECT_FALSE(schedule.anything_scheduled());
    EXPECT_THROW({
        schedule.next_buffer();
    }, std::logic_error);
}

TEST_F(TimeoutQueue, queues_buffers_up)
{
    EXPECT_FALSE(schedule.anything_scheduled());

    for(auto i = 0u; i < num_buffers; i++)
        if (i & 1) schedule.schedule(buffers[i]);
    for(auto i = 0u; i < num_buffers; i++)
        if (!(i & 1)) schedule.schedule(buffers[i]);

    EXPECT_TRUE(schedule.anything_scheduled());

    for(auto i = 0u; i < num_buffers; i++)
    {
        auto buffer = schedule.next_buffer();
        EXPECT_THAT(buffer, Eq(buffers[i]));
    }

    EXPECT_FALSE(schedule.anything_scheduled());
}

TEST_F(TimeoutQueue, queuing_the_same_buffer_moves_it_to_front_of_queue)
{
    for(auto i = 0u; i < num_buffers; i++)
        schedule.schedule(buffers[i]);
    schedule.schedule(buffers[0]);

    std::vector<std::shared_ptr<mg::Buffer>> scheduled_buffers;
    while(schedule.anything_scheduled())
        scheduled_buffers.emplace_back(schedule.next_buffer());

    EXPECT_THAT(scheduled_buffers,
        ElementsAre(buffers[1], buffers[2], buffers[3], buffers[4], buffers[0]));
}

TEST_F(TimeoutQueue, cancelling_a_buffer_removes_it_from_queue)
{
    for(auto i = 0u; i < num_buffers; i++)
        schedule.schedule(buffers[i]);
    schedule.cancel(buffers[2]);

    std::vector<std::shared_ptr<mg::Buffer>> scheduled_buffers;
    while(schedule.anything_scheduled())
        scheduled_buffers.emplace_back(schedule.next_buffer());

    EXPECT_THAT(scheduled_buffers,
        ElementsAre(buffers[0], buffers[1], buffers[3], buffers[4]));
}
