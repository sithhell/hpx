//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_ANY_SENDER_HPP
#define HPX_EXECUTION_ANY_SENDER_HPP

#include <hpx/execution/any_operation.hpp>
#include <hpx/execution/any_receiver.hpp>
#include <hpx/execution/blocking.hpp>
#include <hpx/execution/detail/vtable.hpp>
#include <hpx/execution/tags.hpp>

namespace hpx { namespace execution {
    template <typename values = void(),
        typename error = void(std::exception_ptr), typename... CPOs>
    struct any_sender;

    template <typename... Values, typename Error, typename... CPOs>
    struct any_sender<void(Values...), void(Error), CPOs...>
      : detail::vtable_base<any_operation(tags::connect_t,
                                any_receiver<void(Values...), void(Error)>),
            blocking_kind(tags::blocking_t), CPOs...>
    {
        using vtable_base =
            detail::vtable_base<any_operation(tags::connect_t,
                                    any_receiver<void(Values...), void(Error)>),
                blocking_kind(tags::blocking_t), CPOs...>;

        template <template <typename...> class Tuple,
            template <typename...> class Variant>
        using value_types = Variant<Tuple<Values...>>;

        template <template <typename...> class Variant>
        using error_types = Variant<Error>;

        template <typename Sender,
            typename std::enable_if<
                !std::is_same<any_sender,
                    typename std::decay<Sender>::type>::value,
                int>::type = 0>
        any_sender(Sender&& sender)
          : vtable_base(new
                typename std::decay<Sender>::type(std::forward<Sender>(sender)))
        {
        }

        any_sender(any_sender&& other) noexcept = default;
        any_sender& operator=(any_sender&& other) noexcept = default;
    };
}}    // namespace hpx::execution

#endif
