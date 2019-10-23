//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SYNC_BULK_EXECUTE_HPP
#define HPX_EXECUTION_SYNC_BULK_EXECUTE_HPP

#include <hpx/execution/async_bulk_execute.hpp>
#include <hpx/execution/sync_get.hpp>
#include <hpx/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct sync_bulk_execute_t
        {
            template <typename Executor, typename N, typename F, typename... Ts>
            constexpr auto operator()(
                Executor&& exec, N&& n, F&& f, Ts&&... ts) const
                noexcept(is_tag_invocable_nothrow<sync_bulk_execute_t, Executor,
                    N, F, Ts...>::value)
                    -> hpx::tag_invoke_result_t<sync_bulk_execute_t, Executor,
                        N, F, Ts...>
            {
                return hpx::tag_invoke(*this, std::forward<Executor>(exec),
                    std::forward<N>(n), std::forward<F>(f),
                    std::forward<Ts>(ts)...);
            }

            template <typename Executor, typename N, typename F, typename... Ts>
            constexpr void operator()(Executor&& exec, N&& n, F&& f,
                Ts&&... ts)
                const    //noexcept(is_tag_invocable_nothrow<async_execute_t,
            //Executor, F, std::size_t, Ts...>::value)
            {
                hpx::execution::sync_get(hpx::execution::async_bulk_execute(
                    std::forward<Executor>(exec), std::forward<N>(n),
                    std::forward<F>(f), std::forward<Ts>(ts)...));
            }
        };
    }    // namespace tags

    inline constexpr tags::sync_bulk_execute_t sync_bulk_execute;
}}

#endif
