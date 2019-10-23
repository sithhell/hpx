//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ASYNC_EXECUTE_HPP
#define HPX_EXECUTION_ASYNC_EXECUTE_HPP

#include <hpx/config.hpp>
#include <hpx/execution/executor.hpp>
#include <hpx/execution/receiver.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/set_value.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename Result>
        struct async_execute_sender_result
        {
            template <template <typename...> class Tuple>
            using value_types = Tuple<Result>;
        };

        template <>
        struct async_execute_sender_result<void>
        {
            template <template <typename...> class Tuple>
            using value_types = Tuple<>;
        };

        template <bool>
        struct async_execute_continuation;

        template <>
        struct async_execute_continuation<true>
        {
            template <typename F, typename Receiver>
            HPX_FORCEINLINE void operator()(F&& f, Receiver&& rcv)
            {
                    try
                    {
                        hpx::util::invoke(std::forward<F>(f));
                        hpx::execution::set_value(std::forward<Receiver>(rcv));
                    }
                    catch (...)
                    {
                        hpx::execution::set_error(std::forward<Receiver>(rcv),
                            std::current_exception());
                    }
            }
        };
        template <>
        struct async_execute_continuation<false>
        {
            template <typename F, typename Receiver>
            HPX_FORCEINLINE void operator()(F&& f, Receiver&& rcv)
            {
                try
                {
                    hpx::execution::set_value(std::forward<Receiver>(rcv),
                        hpx::util::invoke(std::forward<F>(f)));
                }
                catch (...)
                {
                    hpx::execution::set_error(
                        std::forward<Receiver>(rcv), std::current_exception());
                }
            }
        };

        template <typename Executor, typename Result, typename... Ts>
        struct async_execute_sender
        {
            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types = Variant<typename async_execute_sender_result<
                Result>::template value_types<Tuple>>;

            template <template <typename...> class Variant>
            using error_types = Variant<std::exception_ptr>;

            using executor_type = typename std::decay<Executor>::type;
            using function_type = hpx::util::detail::deferred<Ts...>;

            async_execute_sender(async_execute_sender&&) noexcept = default;

            template <typename Executor_, typename... Ts_>
            async_execute_sender(Executor_ exec, Ts_&&... ts)
              : exec_(std::move(exec))
              , f_(std::forward<Ts_>(ts)...)
            {
            }

            template <typename Receiver>
            static constexpr bool is_async_execute_sender_receiver(
                std::true_type /*is_void*/)
            {
                return hpx::execution::is_receiver_of<Receiver>::value;
            }

            template <typename Receiver>
            static constexpr bool is_async_execute_sender_receiver(
                std::false_type /*is_void*/)
            {
                return hpx::execution::is_receiver_of<Receiver, Result>::value;
            }

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;
                executor_type exec_;
                function_type f_;
                receiver_type rcv_;

                constexpr hpx::execution::blocking_kind tag_invoke(
                    hpx::execution::tags::blocking_t) const
                {
                    return hpx::execution::blocking(exec_);
                }

                HPX_FORCEINLINE void tag_invoke(hpx::execution::tags::start_t)
                {
                    hpx::execution::execute(exec_,
                        async_execute_continuation<
                            std::is_void<Result>::value>{},
                        std::move(f_), std::move(rcv_));
                }
            };

            template <typename Receiver,
                typename std::enable_if<is_async_execute_sender_receiver<
                                            Receiver>(std::is_void<Result>{}),
                    int>::type = 0>
            HPX_FORCEINLINE operation<Receiver> tag_invoke(
                hpx::execution::tags::connect_t, Receiver&& receiver) &&
            {
                return operation<Receiver>{std::move(exec_), std::move(f_),
                    std::forward<Receiver>(receiver)};
            }

            HPX_FORCEINLINE blocking_kind tag_invoke(
                hpx::execution::tags::blocking_t)
            {
                return hpx::execution::blocking(exec_);
            }

            executor_type exec_;
            function_type f_;
        };
    }    // namespace detail

    namespace tags {
        struct async_execute_t
        {
            template <typename Executor, typename... Ts>
            constexpr auto
            operator()(Executor&& exec, Ts&&... ts) const noexcept(
                is_tag_invocable_nothrow<async_execute_t, Executor,
                    Ts...>::value)
                -> hpx::tag_invoke_result_t<async_execute_t, Executor, Ts...>
            {
                return hpx::tag_invoke(*this, std::forward<Executor>(exec),
                    std::forward<Ts>(ts)...);
            }

            template <typename Executor, typename... Ts,
                typename R = typename std::decay<decltype(
                    hpx::util::invoke(std::declval<Ts>()...))>::type>
            constexpr detail::async_execute_sender<Executor, R, Ts...>
            operator()(Executor&& exec, Ts&&... ts) const
            {
                return detail::async_execute_sender<Executor, R, Ts...>(
                    std::forward<Executor>(exec), std::forward<Ts>(ts)...);
            }
        };
    }    // namespace tags
    inline constexpr tags::async_execute_t async_execute;
}}    // namespace hpx::execution

#endif
