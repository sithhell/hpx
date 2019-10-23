//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_LCOS_LOCAL_NO_MUTEX_HPP
#define HPX_LCOS_LOCAL_NO_MUTEX_HPP

#include <hpx/config.hpp>
#include <hpx/basic_execution/config/defines.hpp>
#include <hpx/no_mutex.hpp>

namespace hpx { namespace lcos { namespace local {
    using no_mutex = hpx::no_mutex;
}}}    // namespace hpx::lcos::local

#if defined(HPX_BASIC_EXECUTION_HAVE_DEPRECATION_WARNINGS)
#if defined(HPX_MSVC)
#pragma message("The header hpx/lcos/local/no_mutex.hpp is deprecated, \
    please include hpx/no_mutext.hpp instead"
#else
#warning "The header hpx/lcos/local/no_mutex.hpp is deprecated, \
    please include hpx/no_mutext.hpp instead"
#endif
#endif

#endif
