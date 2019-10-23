//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SCHEDULER_HPP
#define HPX_EXECUTION_SCHEDULER_HPP

#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <type_traits>

namespace hpx { namespace execution {
    template <typename T>
    struct is_scheduler
      : std::integral_constant<bool,
            std::is_copy_constructible<typename std::remove_cv<
                typename std::remove_reference<T>::type>::type>::value &&
                //std::is_equality_comparable<typename std::remove_cv<
                //    typename std::remove_reference<T>::type>::type>::value &&
                is_tag_invocable<tags::schedule_t, T>::value>
    {
    };
}}    // namespace hpx::execution

#endif
