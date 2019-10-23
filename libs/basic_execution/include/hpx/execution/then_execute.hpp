//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_THEN_EXECUTE_HPP
#define HPX_EXECUTION_THEN_EXECUTE_HPP

#include <hpx/config.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/set_value.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename Receiver, typename F>
        struct then_executed_receiver
        {
            using receiver_type = typename std::decay<Receiver>::type;
            using f_type = typename std::decay<F>::type;

            receiver_type r_;
            f_type f_;

            template <typename _Receiver, typename _F>
            then_executed_receiver(_Receiver&& r, _F&& f)
              : r_(std::forward<_Receiver>(r))
              , f_(std::forward<_F>(f))
            {
            }

            template <typename... Ts>
            HPX_FORCEINLINE void tag_invoke(tags::set_value_t, Ts&&... ts)
            {
                try
                {
                    using f_result = decltype(
                        hpx::util::invoke(f_, std::forward<Ts>(ts)...));
                    set_value_impl(
                        std::is_void<f_result>(), std::forward<Ts>(ts)...);
                }
                catch (...)
                {
                    hpx::execution::set_error(r_, std::current_exception());
                }
            }

            template <typename... Ts>
            HPX_FORCEINLINE constexpr void set_value_impl(
                std::true_type /*is_void*/, Ts&&... ts)
            {
                hpx::util::invoke(f_, std::forward<Ts>(ts)...);
                hpx::execution::set_value(r_);
            }

            template <typename... Ts>
            HPX_FORCEINLINE constexpr void set_value_impl(
                std::false_type /*is_void*/, Ts&&... ts)
            {
                hpx::execution::set_value(
                    r_, hpx::util::invoke(f_, std::forward<Ts>(ts)...));
            }

            HPX_FORCEINLINE constexpr void tag_invoke(tags::set_done_t) noexcept
            {
                hpx::execution::set_done(r_);
            }

            template <typename... Ts>
            HPX_FORCEINLINE constexpr void tag_invoke(
                tags::set_error_t, Ts&&... ts)
            {
                hpx::execution::set_error(r_, std::forward<Ts>(ts)...);
            }
        };

        template <typename Sender, typename F>
        struct then_executed_sender
        {
            using sender_type = typename std::decay<Sender>::type;
            using invocable_type = typename std::decay<F>::type;

            template <template <typename...> class Tuple>
            struct then_sender_values
            {
                template <typename R, typename = void>
                struct impl
                {
                    using type = Tuple<R>;
                };

                template <typename R>
                struct impl<R,
                    typename std::enable_if<std::is_void<R>::value>::type>
                {
                    using type = Tuple<>;
                };

                template <typename... Ts>
                using apply = typename impl<
                    typename hpx::util::invoke_result<F, Ts...>::type>::type;
            };

            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types = typename sender_type::template value_types<
                then_sender_values<Tuple>::template apply, Variant>;

            template <template <typename...> class Variant>
            using error_types =
                typename sender_type::template error_types<Variant>;

            template <typename _Sender, typename _F>
            constexpr then_executed_sender(_Sender&& sender, _F&& f)
              : sender_(std::forward<_Sender>(sender))
              , f_(std::forward<_F>(f))
            {
            }

            then_executed_sender(
                then_executed_sender&& other) noexcept = default;
            then_executed_sender(then_executed_sender const& other) = default;

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;
                sender_type sender_;
                invocable_type f_;
                receiver_type rcv_;

                HPX_FORCEINLINE void tag_invoke(
                    hpx::execution::tags::start_t) &&
                {
                    hpx::execution::submit(std::move(sender_),
                        then_executed_receiver<Receiver, invocable_type>(
                            std::move(rcv_), std::move(f_)));
                }
            };

            template <typename Receiver>    //,
            //                 typename std::enable_if<
            //                     is_receiver_of<Receiver, value_types>::value, int>::type =
            //                     0>
            HPX_FORCEINLINE constexpr operation<Receiver> tag_invoke(
                hpx::execution::tags::connect_t, Receiver&& r) &&
            {
                return operation<Receiver>{std::move(sender_), std::move(f_),
                    std::forward<Receiver>(r)};
            }

            template <typename Receiver>    //,
            //                 typename std::enable_if<
            //                     is_receiver_of<Receiver, value_types>::value, int>::type =
            //                     0>
            HPX_FORCEINLINE constexpr operation<Receiver> tag_invoke(
                hpx::execution::tags::connect_t, Receiver&& r) const&
            {
                return operation<Receiver>{
                    sender_, f_, std::forward<Receiver>(r)};
            }

            template <typename Receiver>    //,
            //                 typename std::enable_if<
            //                     is_receiver_of<Receiver, value_types>::value, int>::type =
            //                     0>
            HPX_FORCEINLINE constexpr operation<Receiver> tag_invoke(
                hpx::execution::tags::connect_t, Receiver&& r) &
            {
                return operation<Receiver>{
                    sender_, f_, std::forward<Receiver>(r)};
            }

            HPX_FORCEINLINE blocking_kind tag_invoke(
                hpx::execution::tags::blocking_t)
            {
                return hpx::execution::blocking(sender_);
            }

            sender_type sender_;
            invocable_type f_;
        };
    }    // namespace detail

    namespace tags {

        struct then_execute_t
        {
            template <typename Sender, typename F,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    is_tag_invocable<then_execute_t, Sender, F>::value>::type>
            constexpr auto operator()(Sender&& sender, F&& f) const noexcept(
                is_tag_invocable_nothrow<then_execute_t, Sender, F>::value)
                -> hpx::tag_invoke_result_t<then_execute_t, Sender, F>
            {
                return hpx::tag_invoke(
                    *this, std::forward<Sender>(sender), std::forward<F>(f));
            }

            // Generic algorithm...
            template <typename Sender, typename F,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    !is_tag_invocable<then_execute_t, Sender, F>::value>::type>
            constexpr detail::then_executed_sender<Sender, F> operator()(
                Sender&& sender, F&& f) const
            {
                return detail::then_executed_sender<Sender, F>(
                    std::forward<Sender>(sender), std::forward<F>(f));
            }
        };
    }    // namespace tags
    inline constexpr tags::then_execute_t then_execute = {};
}}       // namespace hpx::execution

#endif
