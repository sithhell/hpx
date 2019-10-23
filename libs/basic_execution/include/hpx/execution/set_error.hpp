//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SET_ERROR_HPP
#define HPX_EXECUTION_SET_ERROR_HPP

#include <hpx/execution/receiver.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct set_error_t
        {
            template <typename Receiver, typename E>
            constexpr auto operator()(Receiver&& r, E&& e) const noexcept(
                is_tag_invocable_nothrow<set_error_t, Receiver, E>::value)
                -> hpx::tag_invoke_result_t<set_error_t, Receiver, E>
            {
                return hpx::tag_invoke(
                    *this, std::forward<Receiver>(r), std::forward<E>(e));
            }
        };
    }    // namespace tags

    inline constexpr tags::set_error_t set_error;
}}    // namespace hpx::execution

#endif
