//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SENDER_REF_HPP
#define HPX_EXECUTION_SENDER_REF_HPP

#include <hpx/execution/sender.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/tag_invoke.hpp>

#include <memory>
#include <type_traits>

namespace hpx { namespace execution {
    template <typename Sender, bool = is_sender<Sender>::value>
    struct sender_reference_wrapper;

    template <typename Sender>
    struct sender_reference_wrapper<Sender, true>
    {
        using sender_type = typename std::remove_reference<Sender>::type;

        template <template <typename...> class Tuple,
            template <typename...> class Variant>
        using value_types =
            typename sender_type::template value_types<Tuple, Variant>;

        template <template <typename...> class Variant>
        using error_types = typename sender_type::template error_types<Variant>;

        explicit sender_reference_wrapper(Sender& s) noexcept
          : s_(std::addressof(s))
        {
        }

        template <typename CPO, typename... Ts>
        friend constexpr auto tag_invoke(CPO cpo, sender_reference_wrapper& s,
            Ts&&... ts) -> hpx::tag_invoke_result_t<CPO, sender_type&, Ts...>
        {
            return hpx::tag_invoke(cpo, *s.s_, std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend constexpr auto tag_invoke(
            CPO cpo, sender_reference_wrapper const& s, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, sender_type const&, Ts...>
        {
            return hpx::tag_invoke(
                cpo, const_cast<Sender const&>(*s.s_), std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend constexpr auto tag_invoke(CPO cpo, sender_reference_wrapper&& s,
            Ts&&... ts) -> hpx::tag_invoke_result_t<CPO, sender_type&&, Ts...>
        {
            return hpx::tag_invoke(
                cpo, std::move(*s.s_), std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend constexpr auto tag_invoke(
            CPO cpo, sender_reference_wrapper const&& s, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, sender_type const&&, Ts...>
        {
            return hpx::tag_invoke(cpo, const_cast<Sender const&&>(*s.s_),
                std::forward<Ts>(ts)...);
        }

        sender_type* s_;
    };

    template <typename Sender>
    auto sender_ref(Sender& s)
    {
        return sender_reference_wrapper<Sender>(s);
    }
}}    // namespace hpx::execution

#endif
