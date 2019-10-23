//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_EXECUTOR_HPP
#define HPX_EXECUTION_EXECUTOR_HPP

#include <hpx/execution/tags.hpp>
#include <hpx/functional/traits/is_callable.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>

namespace hpx { namespace execution {
    // Invocable archetype
    using invocable_archetype = void();

    namespace detail {
        template <typename E, typename F, typename... Ts>
        struct is_executor_of_impl
          : std::integral_constant<bool,
                hpx::traits::is_invocable<F, Ts...>::value &&
                    std::is_nothrow_constructible<E>::value &&
                    std::is_nothrow_destructible<E>::value &&
                    //std::is_equality_comparable<E>::value &&
                    hpx::is_tag_invocable<tags::execute_t, E, F,
                        Ts...>::value &&
                    hpx::is_tag_invocable<tags::execute_t, E&, F,
                        Ts...>::value &&
                    hpx::is_tag_invocable<tags::execute_t, E const&, F,
                        Ts...>::value &&
                    hpx::is_tag_invocable<tags::execute_t, E&&, F,
                        Ts...>::value &&
                    hpx::is_tag_invocable<tags::execute_t, E const&&, F,
                        Ts...>::value>
        {
        };
    }    // namespace detail

    template <typename E>
    struct is_executor
      : detail::is_executor_of_impl<typename std::decay<E>::type,
            invocable_archetype>
    {
    };

    template <typename E, typename F, typename... Ts>
    struct is_executor_of
      : detail::is_executor_of_impl<typename std::decay<E>::type, F, Ts...>
    {
    };
}}    // namespace hpx::execution

#endif
