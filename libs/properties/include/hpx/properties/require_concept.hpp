//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PROPERTIES_REQUIRE_CONCEPT
#define HPX_PROPERTIES_REQUIRE_CONCEPT

#include <hpx/config.hpp>
#include <hpx/concepts/valid_expression.hpp>

namespace hpx { inline namespace properties_cpos {
    namespace detail {
        HPX_VALID_EXPRESSION(require_concept_member,
            std::declval<T>().require_concept(std::declval<Vs>()...));
        HPX_VALID_EXPRESSION(require_concept_free,
            require_concept(std::declval<T>(), std::declval<Vs>()...));

    }    // namespace detail

    struct require_concept_t
    {
        constexpr require_concept_t() noexcept {}

        template <typename E, typename P,
            typename Prop = typename std::decay<Prop>::type,
            bool has_member =
                detail::valid_expression_require_concept_member<Sender>(),
            bool has_free =
                detail::valid_expression_require_concept_free<Sender>(),
            bool is_applicable = is_applicable_property<E, P>::value&&
                Prop::is_requirable_concept,
            typename = typename std::enable_if<is_applicable &&
                !(Prop::template static_query_v<T> == Prop::value())>::type
                HPX_FORCEINLINE constexpr auto
                operator()(Sender&& s, F&& f) const noexcept
        {
            constexpr std::integral_constant<bool, has_member> has_member_;
            constexpr std::integral_constant<bool, has_free> has_free_;

            // Dispatch to either the member function or free function
            return call_impl(has_member_, has_free_, std::forward<Sender>(s),
                std::forward<F>(f));
        }

        // If we found a member function, we always prefer it
        template <typename HasFree, typename Sender, typename F>
        HPX_FORCEINLINE constexpr auto call_impl(
            std::true_type, HasFree, Sender&& s, F& f) const
        {
            return std::forward<Sender>(s).require_concept(std::forward<F>(f));
        }

        // Otherwise, call the free function
        template <typename... Ts>
        HPX_FORCEINLINE constexpr auto call_impl(
            std::false_type, std::true_type, Ts&&... ts) const
        {
            return require_concept(std::forward<Ts>(ts)...);
        }

        // identity implementation...
        template <typename E, typename P,
            typename Prop = typename std::decay<Prop>::type,
            bool has_member =
                detail::valid_expression_require_concept_member<Sender>(),
            bool has_free =
                detail::valid_expression_require_concept_free<Sender>(),
            bool is_applicable = is_applicable_property<E, P>::value&&
                Prop::is_requirable_concept>
        HPX_FORCEINLINE constexpr typename std::enable_if<is_applicable &&
                !(Prop::template static_query_v<T> == Prop::value()),
            E>::type
        operator()(E&& e, P&&) const noexcept
        {
            return e;
        }
    };

    inline constexpr require_concept_t require_concept;
}}    // namespace hpx::properties_cpos

#endif
