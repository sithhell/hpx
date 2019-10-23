//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_EXCEPTION_HPP
#define HPX_EXECUTION_EXCEPTION_HPP

#include <exception>
#include <stdexcept>

namespace hpx { namespace execution {
    struct receiver_invocation_error
      : std::runtime_error
      , std::nested_exception
    {
        receiver_invocation_error() noexcept;
    };

    struct bad_executor : public std::exception
    {
        bad_executor() noexcept;

        char const* what() const noexcept;
    };
}}    // namespace hpx::execution

#endif
