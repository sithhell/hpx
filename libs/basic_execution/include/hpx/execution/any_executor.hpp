//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_EXECUTOR_HPP
#define HPX_EXECUTION_ANY_EXECUTOR_HPP

#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

#include <exception>

namespace hpx { namespace execution {
    template <typename CPOs...>
    struct any_executor
      : detail::vtable_base<void(tags::execute_t,
                                hpx::util::function_nonser<void()>),
            blocking_kind(tags::blocking_t), CPOs...>
    {
        using vtable_base =
            detail::vtable_base<void(tags::execute_t,
                                    hpx::util::function_nonser<void()>),
                blocking_kind(tags::blocking_t), CPOs...>;

        // FIXME: add allocator support...
        template <typename Executor,
            typename std::enable_if<
                !std::is_same<any_executor,
                    typename std::decay<Receiver>::type>::value,
                int>::type = 0>
        any_executor(Executor&& exec)
          : vtable_base(new typename std::decay<Executor>::type(
                std::forward<Executor>(exec)))
        {
        }

        any_executor(any_executor&& other) noexcept = default;
        any_executor& operator=(any_executor&& other) noexcept = default;
    };

}}    // namespace hpx::execution

#endif
