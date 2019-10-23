//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_CONCEPTS_VALID_EXPRESSION_HPP
#define HPX_CONCEPTS_VALID_EXPRESSION_HPP

#include <hpx/preprocessor/cat.hpp>

#include <type_traits>

/// HPX_VALID_EXPRESSION expands to a constexpr function valid_expression_name
/// taking a variadic set of elements. It checks wether Expr is a valid expression.
/// The first element in the parameter pack is available as type T.
/// Example:
/// HPX_VALID_EXPRESSION(member_f, std::decval<T>().f())
/// struct foo
/// {
///    void f() {}
/// };
/// struct bar
/// {
/// };
/// static_assert(valid_expression_member_f<foo>(), "foo has member f");
/// static_assert(!valid_expression_member_f<bar>(), "bar has no member f");
#define HPX_VALID_EXPRESSION(name, expr)                                       \
    template <typename TT, typename... Vs>                                     \
    struct HPX_PP_CAT(valid_expression_impl_, name)                            \
    {                                                                          \
        template <typename T, typename = decltype(expr)>                       \
        static constexpr bool check_impl(T* t)                                 \
        {                                                                      \
            return true;                                                       \
        }                                                                      \
        template <typename T>                                                  \
        static constexpr bool check_impl(...)                                  \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
        static constexpr bool value = check_impl<TT>(                          \
            static_cast<typename std::decay<TT>::type*>(nullptr));             \
    };                                                                         \
    template <typename... Ts>                                                  \
    constexpr bool HPX_PP_CAT(valid_expression_, name)()                       \
    {                                                                          \
        return HPX_PP_CAT(valid_expression_impl_,                              \
            name)<typename std::decay<Ts>::type...>::value;                    \
    }                                                                          \
    /**/

#define HPX_VALID_EXPRESSION_NOEXCEPT(name, expr)                              \
    template <typename TT, typename... Vs>                                     \
    struct HPX_PP_CAT(valid_expression_impl_, name)                            \
    {                                                                          \
        template <typename T, typename = decltype(expr)>                       \
        static constexpr bool check_impl(T* t)                                 \
        {                                                                      \
            return noexcept(expr);                                             \
        }                                                                      \
        template <typename T>                                                  \
        static constexpr bool check_impl(...)                                  \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
        static constexpr bool value = check_impl<TT>(                          \
            static_cast<typename std::decay<TT>::type*>(nullptr));             \
    };                                                                         \
    template <typename... Ts>                                                  \
    constexpr bool HPX_PP_CAT(valid_expression_, name)()                       \
    {                                                                          \
        return HPX_PP_CAT(valid_expression_impl_,                              \
            name)<typename std::decay<Ts>::type...>::value;                    \
    }                                                                          \
    /**/
#endif
