//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_WHEN_ALL_HPP
#define HPX_EXECUTION_WHEN_ALL_HPP

#include <hpx/datastructures/tuple.hpp>
#include <hpx/execution/connect.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/tags.hpp>

namespace hpx { namespace execution {
    namespace detail {
        template <typename... Ts>
        struct dummy;

        template <typename Tuple>
        struct filter_void;

        template <typename InputTuple, typename OutputTuple>
        struct filter_void_impl;

        template <template <typename...> class Tuple, typename T,
            typename... Us, typename... Ts>
        struct filter_void_impl<Tuple<T, Us...>, Tuple<Ts...>>
        {
            using type =
                typename filter_void_impl<Tuple<Us...>, Tuple<Ts..., T>>::type;
        };

        template <template <typename...> class Tuple, typename... Us,
            typename... Ts>
        struct filter_void_impl<Tuple<void, Us...>, Tuple<Ts...>>
        {
            using type = typename filter_void_impl<Tuple<Us...>,
                Tuple<Ts..., hpx::util::tuple<>>>::type;
        };

        template <template <typename...> class Tuple, typename... Ts>
        struct filter_void_impl<Tuple<>, Tuple<Ts...>>
        {
            using type = Tuple<Ts...>;
        };

        template <template <typename...> class Tuple, typename... Ts>
        struct filter_void<Tuple<Ts...>>
        {
            using type = typename filter_void_impl<Tuple<Ts...>, Tuple<>>::type;
        };

        template <typename... Senders>
        struct when_all_sender
        {
            using senders_type =
                hpx::util::tuple<typename std::decay<Senders>::type...>;

            template <template <typename...> class Tuple>
            struct when_all_result
            {

                template <class T>
                struct convert;
                template <typename... Ts>
                struct convert<dummy<Ts...>>
                {
                    using type = Tuple<Ts...>;
                };

                template <typename... Ts>
                struct impl
                {
                    using type = typename convert<
                        typename filter_void<dummy<Ts...>>::type>::type;
                };

                using apply = typename impl<
                    typename sender_value_type<Senders>::type...>::type;
            };

            template <template <typename...> class Tuple,
                template <typename...> class Variant>
            using value_types = Variant<typename when_all_result<Tuple>::apply>;

            template <template <typename...> class Variant>
            using error_types =
                Variant<typename sender_error_type<Senders>::type...>;

            using index_pack =
                hpx::util::detail::make_index_pack<sizeof...(Senders)>;

            template <typename Receiver>
            struct operation
            {
                template <std::size_t Index>
                struct when_all_receiver
                {
                    template <typename... Ts>
                    void tag_invoke(tags::set_value_t, Ts&&... ts)
                    {
                        this_->set_value<Index>(std::forward<Ts>(ts)...);
                    }

                    template <typename E>
                    void tag_invoke(tags::set_error_t, E&& e)
                    {
                        this_->set_error(std::forward<E>(e));
                    }

                    void tag_invoke(tags::set_done_t)
                    {
                        this_->set_done();
                    }

                    operation* this_;
                };

                template <typename IndexPack>
                struct get_operations_type;
                template <std::size_t... Is>
                struct get_operations_type<
                    hpx::util::detail::pack_c<std::size_t, Is...>>
                {
                    using type = std::tuple<decltype(hpx::execution::connect(
                        hpx::util::get<Is>(std::declval<senders_type&&>()),
                        std::declval<when_all_receiver<Is>>()))...>;
                };

                using receiver_type = typename std::decay<Receiver>::type;
                using operations_type = typename get_operations_type<
                    typename index_pack::type>::type;

                template <typename... Ts>
                using calculate_result_tuple =
                    hpx::util::tuple<hpx::util::optional<Ts>...>;

                using result_value_type =
                    typename value_types<calculate_result_tuple,
                        detail::single_variant>::type;

                receiver_type rcv_;
                operations_type ops_;
                std::atomic<bool> operation_done_;
                std::atomic<bool> operation_error_;
                std::atomic<std::size_t> running_;
                result_value_type results_;

                template <typename _Receiver, std::size_t... Is>
                operation(_Receiver&& rcv, senders_type&& senders,
                    hpx::util::detail::pack_c<std::size_t, Is...>)
                  : rcv_(std::forward<_Receiver>(rcv))
                  , ops_(hpx::execution::connect(
                        hpx::util::get<Is>(std::move(senders)),
                        when_all_receiver<Is>{this})...)
                  , operation_done_(false)
                  , operation_error_(false)
                  , running_(sizeof...(Senders))
                {
                }

                HPX_FORCEINLINE void tag_invoke(tags::start_t)
                {
                    start_impl(index_pack{});
                }

                template <std::size_t... Is>
                HPX_FORCEINLINE void start_impl(
                    hpx::util::detail::pack_c<std::size_t, Is...>)
                {
                    int sequencer[] = {0,
                        (hpx::execution::start(
                             hpx::util::get<Is>(std::move(ops_))),
                            0)...};
                    (void) sequencer;
                }

                template <std::size_t Index, typename... Ts>
                HPX_FORCEINLINE void set_value(Ts&&... ts)
                {
                    hpx::util::get<Index>(results_).emplace(
                        std::forward<Ts>(ts)...);
                    element_complete();
                }
                template <std::size_t Index>
                HPX_FORCEINLINE void set_value()
                {
                    hpx::util::get<Index>(results_).emplace(
                        hpx::util::tuple<>{});
                    element_complete();
                }

                template <typename E>
                void set_error(E&& e)
                {
                    operation_error_.store(true, std::memory_order_release);
                    element_complete();
                }

                void set_done()
                {
                    operation_done_.store(true, std::memory_order_release);
                    element_complete();
                }

                HPX_FORCEINLINE void element_complete()
                {
                    if (running_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    {
                        deliver_results(index_pack{});
                    }
                }

                template <std::size_t... Is>
                HPX_FORCEINLINE void deliver_results(
                    hpx::util::detail::pack_c<std::size_t, Is...>)
                {
                    if (operation_done_.load(std::memory_order_acquire))
                    {
                        hpx::execution::set_done(rcv_);
                        return;
                    }
                    if (operation_error_.load(std::memory_order_acquire))
                    {
                        // TODO: FIXME...
                        std::terminate();
                    }

                    hpx::execution::set_value(
                        rcv_, *hpx::util::get<Is>(std::move(results_))...);
                }
            };

            template <typename... _Senders>
            when_all_sender(_Senders&&... senders)
              : senders_(std::forward<_Senders>(senders)...)
            {
            }

            template <typename Receiver>
            operation<Receiver> tag_invoke(
                tags::connect_t, Receiver&& receiver) &&
            {
                return operation<Receiver>(std::forward<Receiver>(receiver),
                    std::move(senders_), index_pack{});
            }

            HPX_FORCEINLINE blocking_kind tag_invoke(
                hpx::execution::tags::blocking_t)
            {
                return blocking_impl(index_pack{});
            }

            template <std::size_t... Is>
            blocking_kind blocking_impl(
                hpx::util::detail::pack_c<std::size_t, Is...>)
            {
                return blocking_kind::always_inline;
            }

            senders_type senders_;
        };
    }    // namespace detail

    template <typename... Senders>
    detail::when_all_sender<Senders...> when_all(Senders&&... senders)
    {
        return detail::when_all_sender<Senders...>(
            std::forward<Senders>(senders)...);
    }
}}    // namespace hpx::execution

#endif
