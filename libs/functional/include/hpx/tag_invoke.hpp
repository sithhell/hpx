//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_TAG_INVOKE_HPP
#define HPX_TAG_INVOKE_HPP

#if defined(DOXYGEN)
namespace hpx {
    inline namespace unspecified {
        /// The `hpx::tag_invoke` name defines a constexpr opbject that is invocable
        /// with one or more arguments. The first argument is a 'tag' (typically a
        /// CPO). It is only invocable if an overload of tag_invoke() that accepts
        /// the same arguments could be found via ADL.
        ///
        /// The evaluation of the expression `hpx::tag_invoke(tag, args...)` is
        /// equivalent to evaluating the unqualified call to
        /// `tag_invoke(decay-copy(tag), std::forward<Args>(args)...)`.
        ///
        /// `hpx::tag_invoke` is implemented against P1895.
        inline constexpr unspecified tag_invoke = unspecified;
    }    // namespace unspecified

    template <auto& Tag>
    using tag_t = decltype(Tag);

    /// is_tag_invocable<Tag, Args...> is std::true_type if an overload of
    /// `tag_invoke(tag, args...)` can be found via ADL.
    template <typename Tag, typename... Args>
    struct is_tag_invocable;

    /// is_nothrow_tag_invocable<Tag, Args...> is std::true_type if an overload of
    /// `tag_invoke(tag, args...)` can be found via ADL and is noexcept.
    template <typename Tag, typename... Args>
    struct is_nothrow_tag_invocable;

    /// `tag_invoke_result<Tag, Args...>` is the trait returning the result type
    /// of the call hpx::tag_invoke. This can be used in a SFINAE context.
    template <typename Tag, typename... Args>
    using tag_invoke_result = invoke_result<decltype(tag_invoke), Tag, Args...>;

    /// `tag_invoke_result_t<Tag, Args...>` is the trait returning the result type
    /// of the call hpx::tag_invoke. This can be used in a SFINAE context.
    template <typename Tag, typename... Args>
    using tag_invoke_result_t = typename tag_invoke_result<Tag, Args...>::type;
}    // namespace hpx
#else

#include <hpx/config.hpp>
#include <hpx/functional/traits/is_callable.hpp>

namespace hpx {
    namespace tag_invoke_t_ns {
        void tag_invoke() = delete;

        struct tag_invoke_t
        {
            template <typename Tag, typename T, typename... Args>
            constexpr HPX_FORCEINLINE auto operator()(
                Tag&& tag, T&& t, Args&&... args) const
                noexcept(
                    noexcept(tag_invoke(typename std::decay<Tag>::type(tag),
                        std::forward<T>(t), std::forward<Args>(args)...)))
                    -> decltype(tag_invoke(typename std::decay<Tag>::type(tag),
                        std::forward<T>(t), std::forward<Args>(args)...))
            {
                using decay_arg = typename std::decay<Tag>::type;
                return tag_invoke(decay_arg(tag), std::forward<T>(t),
                    std::forward<Args>(args)...);
            }

            template <typename Tag, typename T, typename... Args>
            constexpr HPX_FORCEINLINE auto operator()(
                Tag&& tag, T&& t, Args&&... args) const
                noexcept(noexcept(std::forward<T>(t).tag_invoke(
                    typename std::decay<Tag>::type(tag),
                    std::forward<Args>(args)...)))
                    -> decltype(std::forward<T>(t).tag_invoke(
                        typename std::decay<Tag>::type(tag),
                        std::forward<Args>(args)...))
            {
                using decay_arg = typename std::decay<Tag>::type;
                return std::forward<T>(t).tag_invoke(
                    decay_arg(tag), std::forward<Args>(args)...);
            }

            friend bool operator==(tag_invoke_t, tag_invoke_t)
            {
                return true;
            }
            friend bool operator!=(tag_invoke_t, tag_invoke_t)
            {
                return false;
            }
        };
    }    // namespace tag_invoke_t_ns

    inline namespace tag_invoke_ns {
        inline constexpr tag_invoke_t_ns::tag_invoke_t tag_invoke = {};
    }    // namespace tag_invoke_ns

    template <auto& Tag>
    using tag_t = decltype(Tag);

    template <typename Tag, typename... Args>
    struct is_tag_invocable
      : hpx::traits::is_invocable<decltype(tag_invoke), Tag, Args...>
    {
    };

    namespace detail {
        template <bool, typename Tag, typename... Args>
        struct is_tag_invocable_nothrow_impl;

        template <typename Tag, typename... Args>
        struct is_tag_invocable_nothrow_impl<false, Tag, Args...>
          : std::false_type
        {
        };

        template <typename Tag, typename... Args>
        struct is_tag_invocable_nothrow_impl<true, Tag, Args...>
          : std::integral_constant<bool,
                noexcept(hpx::tag_invoke(
                    std::declval<Tag>(), std::declval<Args>()...))>
        {
        };

    }    // namespace detail

    template <typename Tag, typename... Args>
    struct is_tag_invocable_nothrow
      : detail::is_tag_invocable_nothrow_impl<
            is_tag_invocable<Tag, Args...>::value, Tag, Args...>
    {
    };

    template <typename Tag, typename... Args>
    struct tag_invoke_result
      : hpx::util::invoke_result<decltype(tag_invoke), Tag, Args...>
    {
    };

    template <typename Tag, typename... Args>
    using tag_invoke_result_t = typename tag_invoke_result<Tag, Args...>::type;
}    // namespace hpx

#endif
#endif

