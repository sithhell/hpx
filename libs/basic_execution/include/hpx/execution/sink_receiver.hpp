//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SINK_RECEIVER_HPP
#define HPX_EXECUTION_SINK_RECEIVER_HPP

#include <hpx/config/attributes.hpp>
#include <hpx/execution/tags.hpp>

namespace hpx { namespace execution {
    struct sink_receiver
    {
        template <typename... Ts>
        void tag_invoke(tags::set_value_t, Ts&&...) const
        {
        }

        template <typename T>
        HPX_NORETURN void tag_invoke(
            hpx::execution::tags::set_error_t, T&&) const noexcept
        {
            std::terminate();
        }

        void tag_invoke(hpx::execution::tags::set_done_t) const noexcept {}
    };
}}    // namespace hpx::execution

#endif
