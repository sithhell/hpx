//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_RECEIVER_REF_HPP
#define HPX_EXECUTION_RECEIVER_REF_HPP

#include <hpx/config.hpp>
#include <hpx/execution/receiver.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/set_value.hpp>

#include <memory>
#include <type_traits>

namespace hpx { namespace execution {
    template <typename Receiver, bool = is_receiver<Receiver>::value>
    struct receiver_reference_wrapper;

    template <typename Receiver>
    struct receiver_reference_wrapper<Receiver, true>
    {
        using receiver_type = typename std::decay<Receiver>::type;

        HPX_FORCEINLINE explicit receiver_reference_wrapper(
            Receiver& ref) noexcept
          : ref_(std::addressof(ref))
        {
        }

        receiver_reference_wrapper(
            receiver_reference_wrapper const&) noexcept = default;

        template <typename CPO, typename... Ts>
        friend HPX_FORCEINLINE constexpr auto tag_invoke(
            CPO cpo, receiver_reference_wrapper& r, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, receiver_type&, Ts...>
        {
            return hpx::tag_invoke(cpo, *r.ref_, std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend HPX_FORCEINLINE constexpr auto tag_invoke(
            CPO cpo, receiver_reference_wrapper const& r, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, receiver_type const&, Ts...>
        {
            return hpx::tag_invoke(cpo, const_cast<Receiver const&>(*r.ref_),
                std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend HPX_FORCEINLINE constexpr auto tag_invoke(
            CPO cpo, receiver_reference_wrapper&& r, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, receiver_type&&, Ts...>
        {
            return hpx::tag_invoke(
                cpo, std::move(*r.ref_), std::forward<Ts>(ts)...);
        }

        template <typename CPO, typename... Ts>
        friend HPX_FORCEINLINE constexpr auto tag_invoke(
            CPO cpo, receiver_reference_wrapper const&& r, Ts&&... ts)
            -> hpx::tag_invoke_result_t<CPO, receiver_type const&&, Ts...>
        {
            return hpx::tag_invoke(cpo, const_cast<Receiver const&&>(*r.ref_),
                std::forward<Ts>(ts)...);
        }

        receiver_type* ref_;
    };

    template <typename Receiver>
    HPX_FORCEINLINE auto receiver_ref(Receiver& ref)
    {
        return receiver_reference_wrapper<Receiver>(ref);
    }
}}    // namespace hpx::execution

#endif
