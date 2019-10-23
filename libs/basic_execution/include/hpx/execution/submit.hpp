//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SUBMIT_HPP
#define HPX_EXECUTION_SUBMIT_HPP

#include <hpx/config.hpp>
#include <hpx/execution/connect.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/start.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename Receiver>
        struct as_invocable_t;

        template <typename T, typename Receiver, bool enable>
        struct submit_to_execute;

        template <typename T, typename Receiver>
        struct submit_to_execute<T, Receiver, true>
          : is_executor_of<T,
                detail::as_invocable_t<typename std::decay<Receiver>::type>>
        {
        };
        template <typename T, typename Receiver>
        struct submit_to_execute<T, Receiver, false> : std::false_type
        {
        };

    }    // namespace detail

    namespace tags {

        struct submit_t
        {
            template <typename Sender, typename Receiver>
            HPX_FORCEINLINE constexpr auto operator()(
                Sender&& sender, Receiver&& r) const
                noexcept(
                    is_tag_invocable_nothrow<submit_t, Sender, Receiver>::value)
                    -> hpx::tag_invoke_result_t<submit_t, Sender, Receiver>
            {
                return hpx::tag_invoke(*this, std::forward<Sender>(sender),
                    std::forward<Receiver>(r));
            }

            template <typename Sender, typename Receiver,
                bool is_receiver = is_receiver<Receiver>::value,
                bool is_tag_invocable =
                    is_tag_invocable<submit_t, Sender, Receiver>::value>
            HPX_FORCEINLINE constexpr
                typename std::enable_if<is_receiver && !is_tag_invocable>::type
                operator()(Sender&& sender, Receiver&& receiver) const
            {
                auto op = hpx::execution::connect(std::forward<Sender>(sender),
                    std::forward<Receiver>(receiver));
                hpx::execution::start(std::move(op));
            }

            // If there is no submit customization defined, forward to execution::execute
            template <typename Executor, typename Receiver,
                bool is_executor = detail::submit_to_execute<Executor, Receiver,
                    !is_receiver<Receiver>::value>::value>
            constexpr typename std::enable_if<is_executor>::type operator()(
                Executor&& exec, Receiver&& r) const;
        };
    }    // namespace tags

    inline constexpr tags::submit_t submit;
}}    // namespace hpx::execution

#endif
