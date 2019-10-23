//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_RECEIVER_REF_HPP
#define HPX_EXECUTION_ANY_RECEIVER_REF_HPP

#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

#include <exception>

namespace hpx { namespace execution {
    template <typename Values = void(),
        typename Error = void(std::exception_ptr)>
    struct any_receiver_ref;

    template <typename... Values, typename Error>
    struct any_receiver_ref<void(Values...), void(Error)>
    {
        using vtable_type = detail::vtable<void(tags::set_value_t, Values...),
            void(tags::set_error_t, Error), void(tags::set_done_t)>;

        template <typename Receiver>
        any_receiver_ref(Receiver& r)
          : rcv_(std::addressof(r))
          , vtable_(vtable_type::template create<Receiver>())
        {
        }

        any_receiver_ref(any_receiver_ref const&) noexcept = default;
        any_receiver_ref& operator=(any_receiver_ref const&) noexcept = default;

        template <typename... Ts>
        void tag_invoke(tags::set_value_t, Ts&&... ts)
        {
            vtable_->tag_invoke(
                hpx::execution::set_value, rcv_, std::forward<Ts>(ts)...);
        }

        template <typename E>
        void tag_invoke(tags::set_error_t, E&& e)
        {
            vtable_->tag_invoke(
                hpx::execution::set_error, rcv_, std::forward<E>(e));
        }

        void tag_invoke(tags::set_done_t)
        {
            vtable_->tag_invoke(hpx::execution::set_done, rcv_);
        }

    private:
        void* rcv_;
        vtable_type const* vtable_;
    };

}}    // namespace hpx::execution

#endif
