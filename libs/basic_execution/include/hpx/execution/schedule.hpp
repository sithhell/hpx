//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SCHEDULE_HPP
#define HPX_EXECUTION_SCHEDULE_HPP

#include <hpx/execution/execute.hpp>
#include <hpx/execution/executor.hpp>
#include <hpx/execution/scheduler.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/tags.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename Executor>
        struct executor_sender
        {
            using executor_type = typename std::decay<Executor>::type;

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;
                executor_type exec_;
                receiver_type rcv_;

                void tag_invoke(tags::start_t) && noexcept
                {
                    try
                    {
                        hpx::execution::execute(
                            exec_, hpx::execution::set_value, std::move(rcv_));
                    }
                    catch (...)
                    {
                        hpx::execution::execute(exec_,
                            hpx::execution::set_error, std::move(rcv_),
                            std::current_exception());
                    }
                }
            };

            executor_type exec_;

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& rcv) &
            {
                return operation<Receiver>{exec_, std::forward<Receiver>(rcv)};
            }

            template <typename Receiver>
            operation<Receiver> tag_invoke(
                tags::connect_t, Receiver&& rcv) const&
            {
                return operation<Receiver>{exec_, std::forward<Receiver>(rcv)};
            }

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& rcv) &&
            {
                return operation<Receiver>{
                    std::move(exec_), std::forward<Receiver>(rcv)};
            }
        };
    }    // namespace detail

    namespace tags {
        struct schedule_t
        {
            template <typename Scheduler,
                typename = typename std::enable_if<
                    (!is_sender<Scheduler>::value &&
                        !is_executor<Scheduler>::value) &&
                    is_sender<tag_invoke_result_t<schedule_t,
                        Scheduler>>::value>::type>
            constexpr auto operator()(Scheduler&& s) const
                noexcept(is_tag_invocable_nothrow<schedule_t, Scheduler>::value)
                    -> hpx::tag_invoke_result_t<schedule_t, Scheduler>
            {
                return hpx::tag_invoke(*this, std::forward<Scheduler>(s));
            }

            template <typename Executor,
                typename R = typename std::enable_if<
                    !is_sender<Executor>::value && is_executor<Executor>::value,
                    detail::executor_sender<Executor>>::type>
            constexpr R operator()(Executor&& exec) const
            {
                return detail::executor_sender<Executor>{
                    std::forward<Executor>(exec)};
            }

            template <typename Sender,
                typename R = typename std::decay<Sender>::type,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    !is_executor<Sender>::value>::type>
            constexpr R operator()(Sender&& s) const
            {
                return std::forward<Sender>(s);
            }
        };
    }    // namespace tags

    inline constexpr tags::schedule_t schedule;
}}    // namespace hpx::execution

#endif
