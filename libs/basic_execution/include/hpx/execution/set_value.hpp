//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SET_VALUE_HPP
#define HPX_EXECUTION_SET_VALUE_HPP

#include <hpx/config.hpp>
#include <hpx/execution/receiver.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct set_value_t
        {
            template <typename Receiver, typename... Ts>
            HPX_FORCEINLINE constexpr auto
            operator()(Receiver&& r, Ts&&... ts) const noexcept(
                is_tag_invocable_nothrow<set_value_t, Receiver, Ts...>::value)
                -> hpx::tag_invoke_result_t<set_value_t, Receiver, Ts...>
            {
                return hpx::tag_invoke(
                    *this, std::forward<Receiver>(r), std::forward<Ts>(ts)...);
            }
        };
    }    // namespace tags

    inline constexpr tags::set_value_t set_value;
}}    // namespace hpx::execution

#endif
