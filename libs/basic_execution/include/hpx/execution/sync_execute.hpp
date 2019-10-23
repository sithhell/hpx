//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SYNC_EXECUTE_HPP
#define HPX_EXECUTION_SYNC_EXECUTE_HPP

#include <hpx/execution/async_execute.hpp>
#include <hpx/execution/sync_get.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct sync_execute_t
        {
            template <typename Executor, typename... Ts>
            constexpr auto operator()(Executor&& exec, Ts&&... ts) const
                noexcept(is_tag_invocable_nothrow<sync_execute_t, Executor,
                    Ts...>::value)
                    -> hpx::tag_invoke_result_t<sync_execute_t, Executor, Ts...>
            {
                return hpx::tag_invoke(*this, std::forward<Executor>(exec),
                    std::forward<Ts>(ts)...);
            }

            template <typename Executor, typename... Ts,
                typename AsyncResult = typename hpx::util::invoke_result<
                    async_execute_t, Executor, Ts...>::type,
                typename R = typename hpx::util::invoke_result<sync_get_t,
                    AsyncResult>::type>
            constexpr R operator()(Executor&& exec, Ts&&... ts) const
            {
                return hpx::execution::sync_get(hpx::execution::async_execute(
                    std::forward<Executor>(exec), std::forward<Ts>(ts)...));
            }
        };
    }    // namespace tags
    inline constexpr tags::sync_execute_t sync_execute;
}}    // namespace hpx::execution

#endif
