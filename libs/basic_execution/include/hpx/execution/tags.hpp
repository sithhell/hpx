//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_TAGS_HPP
#define HPX_EXECUTION_TAGS_HPP

namespace hpx { namespace execution { namespace tags {
    // Sender specific tags
    struct connect_t;
    struct is_ready_t;
    struct submit_t;
    struct sync_get_t;
    struct sync_wait_t;

    // Operation specific tags
    struct start_t;

    // Receiver Specific tags
    struct set_value_t;
    struct set_error_t;
    struct set_done_t;

    // Executor specific tags
    struct async_execute_t;
    struct async_bulk_execute_t;
    struct bulk_execute_t;
    struct execute_t;
    struct schedule_t;
    struct sync_execute_t;
    struct sync_bulk_execute_t;
    struct via_t;

    // Properties specific tags
    struct blocking_t;
    struct bulk_guarantee_t;
    struct get_allocator_t;
    struct relationship_t;
}}}    // namespace hpx::execution::tags

#endif
