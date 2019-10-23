//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_RECEIVER_HPP
#define HPX_EXECUTION_ANY_RECEIVER_HPP

#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

#include <exception>

namespace hpx { namespace execution {
    template <typename Values = void(),
        typename Error = void(std::exception_ptr)>
    struct any_receiver;

    template <typename... Values, typename Error>
    struct any_receiver<void(Values...), void(Error)>
      : detail::vtable_base<void(tags::set_value_t, Values...),
            void(tags::set_error_t, Error), void(tags::set_done_t)>
    {
        using vtable_base =
            detail::vtable_base<void(tags::set_value_t, Values...),
                void(tags::set_error_t, Error), void(tags::set_done_t)>;

        // FIXME: add allocator support...
        template <typename Receiver,
            typename std::enable_if<
                !std::is_same<any_receiver,
                    typename std::decay<Receiver>::type>::value,
                int>::type = 0>
        any_receiver(Receiver&& r)
          : vtable_base(new
                typename std::decay<Receiver>::type(std::forward<Receiver>(r)))
        {
        }

        any_receiver(any_receiver&& other) noexcept = default;
        any_receiver& operator=(any_receiver&& other) noexcept = default;
    };

}}    // namespace hpx::execution

#endif
