//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_START_HPP
#define HPX_EXECUTION_START_HPP

#include <hpx/execution/operation.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {

        struct start_t
        {
            template <typename Operation>
            constexpr auto operator()(Operation&& op) const
                noexcept(is_tag_invocable_nothrow<start_t, Operation>::value)
                    -> hpx::tag_invoke_result_t<start_t, Operation>
            {
                return hpx::tag_invoke(*this, std::forward<Operation>(op));
            }
        };
    }    // namespace tags

    inline constexpr tags::start_t start;
}}    // namespace hpx::execution

#endif
