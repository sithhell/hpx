//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SENDER_HPP
#define HPX_EXECUTION_SENDER_HPP

#include <hpx/datastructures/tuple.hpp>
// #include <hpx/datastructures/variant.hpp>
#include <hpx/execution/receiver.hpp>
#include <hpx/execution/sink_receiver.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    // Forward declaration
    namespace detail {
        template <typename Sender, typename Receiver,
            bool = is_receiver<Receiver>::value>
        struct is_sender_impl;

        template <typename Sender, typename Receiver>
        struct is_sender_impl<Sender, Receiver, true>
          : std::integral_constant<bool,
                std::is_move_constructible<Sender>::value &&
                    (hpx::is_tag_invocable<tags::connect_t, Sender&,
                         Receiver>::value ||
                        hpx::is_tag_invocable<tags::connect_t, Sender const&,
                            Receiver>::value ||
                        hpx::is_tag_invocable<tags::connect_t, Sender&&,
                            Receiver>::value ||
                        hpx::is_tag_invocable<tags::connect_t, Sender const&&,
                            Receiver>::value)>
        {
        };

        template <typename Sender, typename Receiver>
        struct is_sender_impl<Sender, Receiver, false> : std::false_type
        {
        };
    }    // namespace detail

    // A sender is:
    //  - move constructible
    //  - nothrow move constructible or copy constructible
    //  - requires the following expressions to be valid:
    //    - hpx::execution::set_done((T&&) t)
    //    - hpx::execution::set_done((T&&) t, (E&&) e)
    template <typename Sender>
    struct is_sender
      : detail::is_sender_impl<
            typename std::remove_cv<
                typename std::remove_reference<Sender>::type>::type,
            sink_receiver>
    {
    };

    template <typename Sender, typename Receiver,
        bool sender = is_sender<Sender>::value>
    struct is_sender_to;

    template <typename Sender, typename Receiver>
    struct is_sender_to<Sender, Receiver, true>
      : detail::is_sender_impl<
            typename std::remove_cv<
                typename std::remove_reference<Sender>::type>::type,
            Receiver>
    {
    };

    template <typename Sender, typename Receiver>
    struct is_sender_to<Sender, Receiver, false>
      : std::integral_constant<bool, false>
    {
    };

    template <typename Sender>
    struct sender_traits
    {
        template <template <typename...> class Tuple,
            template <typename...> class Variant>
        using value_types =
            typename Sender::template value_types<Tuple, Variant>;

        template <template <typename...> class Variant>
        using error_types = typename Sender::template error_types<Variant>;
    };

    template <typename Sender>
    struct sender_traits<Sender volatile> : sender_traits<Sender>
    {
    };

    template <typename Sender>
    struct sender_traits<Sender const> : sender_traits<Sender>
    {
    };

    template <typename Sender>
    struct sender_traits<Sender&> : sender_traits<Sender>
    {
    };

    template <typename Sender>
    struct sender_traits<Sender&&> : sender_traits<Sender>
    {
    };

    namespace detail {
        template <typename... Values>
        struct single_variant
        {
            // TODO: FIXME variant should be available...
            //using type = hpx::util::variant<typename Values::type...>;
        };

        template <typename T>
        struct single_variant<T>
        {
            using type = T;
        };

        template <typename... Values>
        struct single_tuple
        {
            using type = hpx::util::tuple<Values...>;
        };

        template <typename T>
        struct single_tuple<T>
        {
            using type = T;
        };

        template <>
        struct single_tuple<>
        {
            using type = void;
        };
    };

    template <typename Sender>
    struct sender_value_type
      : sender_traits<Sender>::template value_types<detail::single_tuple,
            detail::single_variant>::type
    {
    };

    template <typename Sender>
    struct sender_error_type
      : sender_traits<Sender>::template error_types<detail::single_variant>
    {
    };
}}    // namespace hpx::execution

#endif
