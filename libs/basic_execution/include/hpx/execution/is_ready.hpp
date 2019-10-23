//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_IS_READY_HPP
#define HPX_EXECUTION_IS_READY_HPP

#include <hpx/config.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx {
    namespace execution { namespace tags {
        struct is_ready_t
        {
            template <typename Sender,
                typename =
                    typename std::enable_if<is_sender<Sender>::value>::type>
            constexpr auto operator()(Sender&& sender) const
                noexcept(is_tag_invocable_nothrow<is_ready_t, Sender>::value)
                    -> hpx::tag_invoke_result_t<is_ready_t, Sender>
            {
                return hpx::tag_invoke(*this, std::forward<Sender>(sender));
            }
        };
    }    // namespace tags
    inline constexpr tags::is_ready_t is_ready;
}}    // namespace hpx::execution

#endif
