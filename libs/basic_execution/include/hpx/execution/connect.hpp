//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_CONNECT_HPP
#define HPX_EXECUTION_CONNECT_HPP

#include <hpx/config.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {

        struct connect_t
        {
            template <typename Sender, typename Receiver>
            HPX_FORCEINLINE constexpr auto
            operator()(Sender&& sender, Receiver&& r) const noexcept(
                is_tag_invocable_nothrow<connect_t, Sender, Receiver>::value)
                -> hpx::tag_invoke_result_t<connect_t, Sender, Receiver>
            {
                return hpx::tag_invoke(*this, std::forward<Sender>(sender),
                    std::forward<Receiver>(r));
            }
        };
    }    // namespace tags

    inline constexpr tags::connect_t connect;
}}    // namespace hpx::execution

#endif

