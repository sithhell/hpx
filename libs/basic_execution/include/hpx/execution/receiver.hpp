//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_RECEIVER_HPP
#define HPX_EXECUTION_RECEIVER_HPP

#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    // Forward declaration
    namespace detail {
        template <typename T, typename E = std::exception_ptr>
        struct is_receiver_impl
          : std::integral_constant<bool,
                std::is_move_constructible<T>::value &&
                    (std::is_nothrow_move_constructible<T>::value ||
                        std::is_copy_constructible<T>::value) &&
                    hpx::is_tag_invocable<hpx::execution::tags::set_done_t,
                        T>::value &&
                    hpx::is_tag_invocable<hpx::execution::tags::set_error_t, T,
                        E>::value>
        {
        };

        template <typename T, typename... An>
        struct is_receiver_of_impl
          : std::integral_constant<bool,
                is_receiver_impl<T>::value &&
                    hpx::is_tag_invocable<hpx::execution::tags::set_value_t, T,
                        An...>::value>
        {
        };
    }    // namespace detail

    // A receiver is:
    //  - move constructible
    //  - nothrow move constructible or copy constructible
    //  - requires the following expressions to be valid:
    //    - hpx::execution::set_done((T&&) t)
    //    - hpx::execution::set_done((T&&) t, (E&&) e)
    template <typename T, typename E = std::exception_ptr>
    struct is_receiver
      : detail::is_receiver_impl<
            typename std::remove_cv<
                typename std::remove_reference<T>::type>::type,
            E>
    {
    };

    template <typename T, typename... An>
    struct is_receiver_of
      : detail::is_receiver_of_impl<
            typename std::remove_cv<
                typename std::remove_reference<T>::type>::type,
            An...>
    {
    };
}}    // namespace hpx::execution

#endif
