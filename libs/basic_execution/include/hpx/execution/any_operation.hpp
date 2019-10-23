//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_OPERATION_HPP
#define HPX_EXECUTION_ANY_OPERATION_HPP

#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

namespace hpx { namespace execution {
    struct any_operation : detail::vtable_base<void(tags::start_t)>
    {
        using vtable_base = detail::vtable_base<void(tags::start_t)>;

        template <typename Operation,
            typename std::enable_if<
                !std::is_same<any_operation,
                    typename std::decay<Operation>::type>::value,
                int>::type = 0>
        any_operation(Operation&& op)
          : vtable_base(new typename std::decay<Operation>::type(
                std::forward<Operation>(op)))
        {
        }

        any_operation(any_operation&& other) noexcept = default;
        any_operation& operator=(any_operation&& other) noexcept = default;
    };
}}    // namespace hpx::execution

#endif
