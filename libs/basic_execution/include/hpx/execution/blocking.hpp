//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_BLOCKING_HPP
#define HPX_EXECUTION_BLOCKING_HPP

#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {

    enum class blocking_kind
    {
        /// No guarantees made on when and which thread the receiver will be
        /// called
        maybe,
        /// Always completes asynchronously.
        never,
        /// The operation executes concurrently.
        always,
        /// The operation is happening inline
        always_inline
    };

    namespace tags {
        struct blocking_t
        {
            template <typename T>
            constexpr auto operator()(T&& t) const
                noexcept(is_tag_invocable_nothrow<blocking_t, T>::value)
                    -> hpx::tag_invoke_result_t<blocking_t, T>
            {
                return hpx::tag_invoke(*this, std::forward<T>(t));
            }

            template <typename T,
                typename std::enable_if<!is_tag_invocable<blocking_t, T>::value,
                    int>::type = 0>
            constexpr blocking_kind operator()(T&& t) const noexcept
            {
                return blocking_kind::maybe;
            }
        };
    }    // namespace tags

    inline constexpr tags::blocking_t blocking;
}}    // namespace hpx::execution

#endif
