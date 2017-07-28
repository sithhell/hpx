//  Copyright (c) 2015-2017 John Biddiscombe
//  Copyright (c) 2017      Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LIBFABRIC_CONTEXT_HPP
#define HPX_PARCELSET_POLICIES_LIBFABRIC_CONTEXT_HPP

#include <rdma/fabric.h>

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    template <typename This>
    struct context
    {
        fi_context context_;
        This* this_;

        This* operator->()
        {
            return this_;
        }
    };
}}}}

#endif
