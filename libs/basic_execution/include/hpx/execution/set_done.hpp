//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SET_DONE_HPP
#define HPX_EXECUTION_SET_DONE_HPP

#include <hpx/execution/receiver.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct set_done_t
        {
            template <typename Receiver>
            constexpr auto operator()(Receiver&& r) const
                noexcept(is_tag_invocable_nothrow<set_done_t, Receiver>::value)
                    -> hpx::tag_invoke_result_t<set_done_t, Receiver>
            {
                return hpx::tag_invoke(*this, std::forward<Receiver>(r));
            }
        };
    }    // namespace tags

    inline constexpr tags::set_done_t set_done;
}}    // namespace hpx::execution

#endif
