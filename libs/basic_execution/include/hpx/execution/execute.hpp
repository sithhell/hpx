//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_EXECUTE_HPP
#define HPX_EXECUTION_EXECUTE_HPP

#include <hpx/config.hpp>
#include <hpx/datastructures/optional.hpp>
#include <hpx/execution/executor.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/set_value.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename F>
        struct as_receiver_t
        {
            F f_;
            template <typename FF>
            explicit as_receiver_t(FF&& f)
              : f_(std::forward<FF>(f))
            {
            }

            template <typename... Ts>
            typename std::enable_if<
                hpx::traits::is_invocable<F, Ts...>::value>::type
            tag_invoke(tags::set_value_t, Ts&&... ts) const
            {
                hpx::util::invoke(f_, std::forward<Ts>(ts)...);
            }

            template <typename... Ts>
            typename std::enable_if<
                hpx::traits::is_invocable<F, Ts...>::value>::type
            tag_invoke(tags::set_value_t, Ts&&... ts)
            {
                hpx::util::invoke(f_, std::forward<Ts>(ts)...);
            }

            void tag_invoke(
                hpx::execution::tags::set_error_t, std::exception_ptr) const
            {
                std::terminate();
            }

            void tag_invoke(hpx::execution::tags::set_done_t) noexcept {}
        };

        template <typename F>
        auto as_receiver(F&& f)
        {
            return as_receiver_t<typename std::decay<F>::type>(
                std::forward<F>(f));
        }

        template <typename Receiver>
        struct as_invocable_t
        {
            hpx::util::optional<Receiver> r_ = {};

            template <typename T>
            void try_init(T&& r)
            {
                try
                {
                    r_.emplace(std::forward<T>(r));
                }
                catch (...)
                {
                    ::hpx::execution::set_error(r, std::current_exception());
                }
            }

            as_invocable_t(Receiver&& r)
            {
                try_init(std::move_if_noexcept(r));
            }

            as_invocable_t(Receiver& r)
            {
                try_init(r);
            }

            as_invocable_t(as_invocable_t&& other)
            {
                if (other.r_)
                {
                    try_init(std::move_if_noexcept(*other.r_));
                    other.r_.reset();
                }
            }

            ~as_invocable_t()
            {
                if (r_)
                    ::hpx::execution::set_done(*r_);
            }

            void operator()()
            {
                try
                {
                    execution::set_value(*r_);
                }
                catch (...)
                {
                    execution::set_error(*r_, std::current_exception());
                }
                r_.reset();
            }
        };

        template <typename Receiver>
        auto as_invocable(Receiver&& r)
        {
            return as_invocable_t<typename std::decay<Receiver>::type>(
                std::forward<Receiver>(r));
        }
    }    // namespace detail

    namespace tags {
        struct execute_t
        {
            template <typename Executor, typename F, typename... Ts>
            HPX_FORCEINLINE constexpr auto
            operator()(Executor&& exec, F&& f, Ts&&... ts) const noexcept(
                is_tag_invocable_nothrow<execute_t, Executor, F, Ts...>::value)
                -> hpx::tag_invoke_result_t<execute_t, Executor, F, Ts...>
            {
                return hpx::tag_invoke(*this, std::forward<Executor>(exec),
                    std::forward<F>(f), std::forward<Ts>(ts)...);
            }

            template <typename Sender, typename F,
                typename =
                    typename std::enable_if<is_sender<Sender>::value>::type>
            HPX_FORCEINLINE constexpr void operator()(
                Sender&& sender, F&& f) const
            {
                ::hpx::execution::submit(std::forward<Sender>(sender),
                    detail::as_receiver(std::forward<F>(f)));
            }
        };
    }    // namespace tags

    inline constexpr tags::execute_t execute;

    namespace tags {

        template <typename Executor, typename Receiver, bool is_executor>
        constexpr typename std::enable_if<is_executor>::type
        submit_t::operator()(Executor&& exec, Receiver&& r) const
        {
            ::hpx::execution::execute(std::forward<Executor>(exec),
                detail::as_invocable(std::forward<Receiver>(r)));
        }
    }    // namespace tags
}}       // namespace hpx::execution

#endif
