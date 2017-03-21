//  Copyright (c) 2015-2017 John Biddiscombe
//  Copyright (c) 2017      Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LIBFABRIC_SEND_HANDLER_HPP
#define HPX_PARCELSET_POLICIES_LIBFABRIC_SEND_HANDLER_HPP

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    struct send_handler
    {
        virtual ~send_handler()
        {}

        virtual void handle_send_completion() = 0;
    };
}}}}

#endif
