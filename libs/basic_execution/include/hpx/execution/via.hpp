//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_VIA_HPP
#define HPX_EXECUTION_VIA_HPP

#include <hpx/config.hpp>
#include <hpx/execution/execute.hpp>
#include <hpx/execution/executor.hpp>
#include <hpx/execution/scheduler.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/set_value.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/tag_invoke.hpp>

namespace hpx { namespace execution {

    namespace detail {

        template <typename Executor, typename Receiver>
        struct via_receiver_executor
        {
            using executor_type = typename std::decay<Executor>::type;
            using receiver_type = typename std::decay<Receiver>::type;

            executor_type exec_;
            receiver_type rcv_;

            template <typename... Ts>
            void tag_invoke(tags::set_value_t, Ts&&... ts)
            {
                hpx::execution::execute(exec_, hpx::execution::set_value,
                    std::move(rcv_), std::forward<Ts>(ts)...);
            }

            template <typename E>
            void tag_invoke(tags::set_error_t, E&& e)
            {
                hpx::execution::execute(exec_, hpx::execution::set_error,
                    std::move(rcv_), std::forward<E>(e));
            }

            void tag_invoke(tags::set_done_t)
            {
                hpx::execution::execute(
                    exec_, hpx::execution::set_done, std::move(rcv_));
            }
        };

        template <typename Sender, typename Receiver>
        struct via_receiver_scheduler
        {
            using sender_type = typename std::decay<Sender>::type;
            using receiver_type = typename std::decay<Receiver>::type;

            sender_type sender_;
            receiver_type rcv_;

            template <typename Tag, typename... Ts>
            struct receiver
            {
                receiver_type rcv_;
                hpx::util::tuple<typename std::decay<Ts>::type...> tuple_;

                void tag_invoke(hpx::execution::tags::set_value_t)
                {
                    tag_invoke_impl(
                        hpx::util::detail::make_index_pack<sizeof...(Ts)>{});
                }

                template <std::size_t... Is>
                void tag_invoke_impl(
                    hpx::util::detail::pack_c<std::size_t, Is...>)
                {
                    hpx::tag_invoke(Tag{}, rcv_, hpx::util::get<Is>(tuple_)...);
                }

                template <typename E>
                void tag_invoke(hpx::execution::tags::set_error_t, E&& e)
                {
                    hpx::execution::set_error(rcv_, std::forward<E>(e));
                }

                void tag_invoke(hpx::execution::tags::set_done_t)
                {
                    hpx::execution::set_done(rcv_);
                }
            };

            template <typename... Ts>
            void tag_invoke(tags::set_value_t, Ts&&... ts)
            {
                hpx::execution::submit(sender_,
                    receiver<hpx::execution::tags::set_value_t, Ts...>{
                        std::move(rcv_),
                        hpx::util::make_tuple(std::forward<Ts>(ts)...)});
            }

            template <typename E>
            void tag_invoke(tags::set_error_t, E&& e)
            {
                hpx::execution::submit(sender_,
                    receiver<hpx::execution::tags::set_error_t, E>{
                        std::move(rcv_),
                        hpx::util::make_tuple(std::forward<E>(e))});
            }

            void tag_invoke(tags::set_done_t)
            {
                hpx::execution::submit(sender_,
                    receiver<hpx::execution::tags::set_done_t>{
                        std::move(rcv_), hpx::util::tuple<>{}});
            }
        };

        template <typename Sender, typename Executor>
        struct via_executor
        {
            using sender_type = typename std::decay<Sender>::type;
            using executor_type = typename std::decay<Executor>::type;
            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types =
                typename sender_type::template value_types<Tuple, Variant>;
            template <template <typename...> class Variant>
            using error_types =
                typename sender_type::template error_types<Variant>;

            sender_type sender_;
            executor_type exec_;

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;
                sender_type sender_;
                executor_type exec_;
                receiver_type rcv_;

                void tag_invoke(tags::start_t) &&
                {
                    hpx::execution::submit(std::move(sender_),
                        via_receiver_executor<Executor, Receiver>{
                            std::move(exec_), std::forward<Receiver>(rcv_)});
                }
            };

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& r) &&
            {
                return operation<Receiver>{std::move(sender_), std::move(exec_),
                    std::forward<Receiver>(r)};
            }
        };
        template <typename Sender, typename Scheduler>
        struct via_scheduler
        {
            using sender_type = typename std::decay<Sender>::type;
            using scheduler_type = typename std::decay<Scheduler>::type;
            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types =
                typename sender_type::template value_types<Tuple, Variant>;
            template <template <typename...> class Variant>
            using error_types =
                typename sender_type::template error_types<Variant>;

            sender_type sender_;
            scheduler_type scheduler_;

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;
                sender_type sender_;
                scheduler_type scheduler_;
                receiver_type rcv_;

                void tag_invoke(tags::start_t) &&
                {
                    using schedule_sender_type =
                        decltype(hpx::execution::schedule(scheduler_));
                    hpx::execution::submit(std::move(sender_),
                        via_receiver_scheduler<schedule_sender_type, Receiver>{
                            hpx::execution::schedule(scheduler_),
                            std::forward<Receiver>(rcv_)});
                }
            };

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& r) &&
            {
                return operation<Receiver>{std::move(sender_),
                    std::move(scheduler_), std::forward<Receiver>(r)};
            }
        };
    }    // namespace detail

    namespace tags {

        struct via_t
        {
            template <typename Sender, typename T,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    (is_executor<T>::value || is_scheduler<T>::value) &&
                    is_tag_invocable<via_t, Sender, T>::value>::type>
            constexpr auto operator()(Sender&& sender, T&& t) const noexcept(
                is_tag_invocable_nothrow<then_execute_t, Sender, T>::value)
                -> hpx::tag_invoke_result_t<then_execute_t, Sender, T>
            {
                return hpx::tag_invoke(
                    *this, std::forward<Sender>(sender), std::forward<T>(t));
            }

            // Generic algorithm if second argument is Executor...
            template <typename Sender, typename Executor,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    is_executor<Executor>::value &&
                    !is_scheduler<Executor>::value &&
                    !is_tag_invocable<via_t, Sender, Executor>::value>::type>
            constexpr detail::via_executor<Sender, Executor> operator()(
                Sender&& sender, Executor&& exec) const
                noexcept(is_tag_invocable_nothrow<then_execute_t, Sender,
                    Executor>::value)
            {
                return detail::via_executor<Sender, Executor>{
                    std::forward<Sender>(sender), std::forward<Executor>(exec)};
            }

            // Generic algorithm if second argument is Scheduler...
            template <typename Sender, typename Scheduler,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    !is_executor<Scheduler>::value &&
                    is_scheduler<Scheduler>::value &&
                    !is_tag_invocable<via_t, Sender, Scheduler>::value>::type>
            constexpr detail::via_scheduler<Sender, Scheduler> operator()(
                Sender&& sender, Scheduler&& scheduler) const
                noexcept(is_tag_invocable_nothrow<then_execute_t, Sender,
                    Scheduler>::value)
            {
                return detail::via_scheduler<Sender, Scheduler>{
                    std::forward<Sender>(sender),
                    std::forward<Scheduler>(scheduler)};
            }
        };
    }    // namespace tags
    inline constexpr tags::via_t via = {};

}}

#endif
