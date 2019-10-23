//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ASYNC_BULK_EXECUTE_HPP
#define HPX_EXECUTION_ASYNC_BULK_EXECUTE_HPP

#include <hpx/execution/async_execute.hpp>
#include <hpx/execution/receiver_ref.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace detail {
        template <typename Executor, typename N, typename F, typename... Ts>
        struct async_bulk_execute_sender
        {
            using executor_type = typename std::decay<Executor>::type;
            using function_type = typename std::decay<F>::type;
            using args_type =
                hpx::util::tuple<typename std::decay<Ts>::type...>;

            using index_pack =
                hpx::util::detail::make_index_pack<sizeof...(Ts)>;

            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types = Variant<Tuple<>>;

            template <template <typename...> class Variant>
            using error_types = Variant<std::exception_ptr>;

            template <typename Receiver>
            struct operation
            {
                using receiver_type = typename std::decay<Receiver>::type;

                struct operation_receiver
                {
                    template <typename... Us>
                    void tag_invoke(tags::set_value_t, Us&&...)
                    {
                        element_complete();
                    }

                    template <typename E>
                    void tag_invoke(tags::set_error_t, E&& e)
                    {
                        this_->operation_error_.store(
                            true, std::memory_order_release);
                        element_complete();
                    }

                    void tag_invoke(tags::set_done_t)
                    {
                        this_->operation_done_.store(
                            true, std::memory_order_release);
                        element_complete();
                    }

                    void element_complete()
                    {
                        if (this_->running_.fetch_sub(
                                1, std::memory_order_acq_rel) == 1)
                        {
                            deliver();
                        }
                    }

                    void deliver()
                    {
                        if (this_->operation_done_.load(
                                std::memory_order_acquire))
                        {
                            hpx::execution::set_done(this_->rcv_);
                            return;
                        }
                        if (this_->operation_error_.load(
                                std::memory_order_acquire))
                        {
                            // TODO: FIXME...
                            std::terminate();
                        }

                        hpx::execution::set_value(this_->rcv_);
                    }

                    operation* this_;
                };

                void tag_invoke(hpx::execution::tags::start_t)
                {
                    start_impl(index_pack{});
                }

                template <std::size_t... Is>
                void start_impl(hpx::util::detail::pack_c<std::size_t, Is...>)
                {
                    try
                    {
                        for (std::size_t i = 0; i != n_; ++i)
                        {
                            hpx::execution::submit(
                                hpx::execution::async_execute(
                                    exec_, f_, i, hpx::util::get<Is>(args_)...),
                                operation_receiver{this});
                        }
                    }
                    catch (...)
                    {
                        hpx::execution::set_error(
                            rcv_, std::current_exception());
                    }
                }

                template <typename _Executor, typename _F, typename Args,
                    typename _Receiver>
                operation(_Executor&& exec, std::size_t n, _F&& f, Args&& args,
                    _Receiver&& rcv)
                  : exec_(std::forward<_Executor>(exec))
                  , n_(n)
                  , f_(std::forward<_F>(f))
                  , args_(std::forward<Args>(args))
                  , rcv_(std::forward<_Receiver>(rcv))
                  , running_(n)
                  , operation_done_(false)
                  , operation_error_(false)
                {}

                executor_type exec_;
                std::size_t n_;
                function_type f_;
                args_type args_;
                receiver_type rcv_;
                std::atomic<std::size_t> running_;
                std::atomic<bool> operation_done_;
                std::atomic<bool> operation_error_;
            };

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& r) &&
            {
                return operation<Receiver>(std::move(exec_), n_, std::move(f_),
                    std::move(args_), std::forward<Receiver>(r));
            }

            template <typename Receiver>
            operation<Receiver> tag_invoke(tags::connect_t, Receiver&& r) const&
            {
                return operation<Receiver>{
                    exec_, n_, f_, args_, std::forward<Receiver>(r)};
            }

            template <typename _Executor, typename _F, typename... _Ts>
            async_bulk_execute_sender(
                _Executor&& exec, std::size_t n, _F&& f, _Ts&&... ts)
              : exec_(std::forward<_Executor>(exec))
              , n_(n)
              , f_(std::forward<_F>(f))
              , args_(std::forward<Ts>(ts)...)
            {
            }

            executor_type exec_;
            std::size_t n_;
            function_type f_;
            args_type args_;
        };
    }    // namespace detail

    namespace tags {
        struct async_bulk_execute_t
        {
            template <typename Executor, typename N, typename F, typename... Ts>
            constexpr auto operator()(
                Executor&& exec, N&& n, F&& f, Ts&&... ts) const
                noexcept(is_tag_invocable_nothrow<async_bulk_execute_t,
                    Executor, N, F, Ts...>::value)
                    -> hpx::tag_invoke_result_t<async_bulk_execute_t, Executor,
                        N, F, Ts...>
            {
                return hpx::tag_invoke(*this, std::forward<Executor>(exec),
                    std::forward<N>(n), std::forward<F>(f),
                    std::forward<Ts>(ts)...);
            }

            template <typename Executor, typename N, typename F, typename... Ts>
            constexpr detail::async_bulk_execute_sender<Executor, N, F, Ts...>
            operator()(Executor&& exec, N&& n, F&& f,
                Ts&&... ts)
                const    //noexcept(is_tag_invocable_nothrow<async_execute_t,
            //Executor, F, std::size_t, Ts...>::value)
            {
                return detail::async_bulk_execute_sender<Executor, N, F, Ts...>(
                    std::forward<Executor>(exec), std::forward<N>(n),
                    std::forward<F>(f), std::forward<Ts>(ts)...);
            }
        };
    }    // namespace tags

    inline constexpr tags::async_bulk_execute_t async_bulk_execute;
}}       // namespace hpx::execution

#endif
