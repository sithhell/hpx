//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_SENDER_REF_HPP
#define HPX_EXECUTION_ANY_SENDER_REF_HPP

#include <hpx/execution/any_operation.hpp>
#include <hpx/execution/any_receiver.hpp>
#include <hpx/execution/blocking.hpp>
#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

#include <exception>

namespace hpx { namespace execution {
    template <typename values = void(),
        typename error = void(std::exception_ptr)>
    struct any_sender_ref;

    template <typename... Values, typename Error>
    struct any_sender_ref<void(Values...), void(Error)>
    {
        using receiver_type = any_receiver<void(Values...), void(Error)>;

        using vtable_type =
            detail::vtable<any_operation(tags::connect_t, receiver_type),
                blocking_kind(tags::blocking_t)>;

        template <template <typename...> class Tuple,
            template <typename...> class Variant>
        using value_types = Variant<Tuple<Values...>>;

        template <template <typename...> class Variant>
        using error_types = Variant<Error>;

        template <typename Sender>
        any_sender_ref(Sender& s)
          : sender_(std::addressof(s))
          , vtable_(vtable_type::template create<Sender>())
        {
        }

        any_sender_ref(any_sender_ref const&) noexcept = default;
        any_sender_ref& operator=(any_sender_ref const&) noexcept = default;

        template <typename Receiver>
        any_operation tag_invoke(tags::connect_t, Receiver&& rcv)
        {
            return vtable_->tag_invoke(
                hpx::execution::connect, sender_, std::forward<Receiver>(rcv));
        }

        blocking_kind tag_invoke(tags::blocking_t) const
        {
            return vtable_->tag_invoke(hpx::execution::blocking, sender_);
        }

    private:
        void* sender_;
        vtable_type const* vtable_;
    };

}}    // namespace hpx::execution

#endif
