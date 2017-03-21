//  Copyright (c) 2015-2017 John Biddiscombe
//  Copyright (c) 2017      Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LIBFABRIC_RECEIVER_HPP
#define HPX_PARCELSET_POLICIES_LIBFABRIC_RECEIVER_HPP

#include <plugins/parcelport/libfabric/libfabric_memory_region.hpp>
#include <plugins/parcelport/libfabric/rdma_memory_pool.hpp>
#include <plugins/parcelport/libfabric/rma_receiver.hpp>

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    struct parcelport;
    // The receiver is responsible for handling incoming messages. For that purpose,
    // it posts receive buffers. Incoming messages can be of two kinds:
    //      1) An ACK message which has been sent from an rma_receiver, to signal
    //         the sender about the succesful retreival of an incoming message.
    //      2) An incoming parcel, that consists of an header and an eventually
    //         piggy backed message. If the message is not piggy backed or zero
    //         copy RMA chunks need to be read, a rma_receiver is created to
    //         complete the transfer of the message
    struct receiver
    {
        receiver()
          : region_(nullptr)
          , memory_pool_(nullptr)
        {}

        receiver(parcelport* pp, fid_ep* endpoint, rdma_memory_pool& memory_pool)
          : region_(memory_pool.allocate_region(memory_pool.small_.chunk_size()))
          , pp_(pp)
          , endpoint_(endpoint)
          , memory_pool_(&memory_pool)
          , rma_receiver_(pp, endpoint, &memory_pool)
        {
            LOG_DEVEL_MSG("created receiver: " << this);
            // Once constructed, we need to post the receive...
            post_recv();
        }

        ~receiver()
        {
            if (region_ && memory_pool_)
                memory_pool_->deallocate(region_);
        }

        receiver(receiver&& other)
          : region_(other.region_)
          , pp_(other.pp_)
          , endpoint_(other.endpoint_)
          , memory_pool_(other.memory_pool_)
          , rma_receiver_(std::move(other.rma_receiver_))
        {
            other.region_ = nullptr;
            other.memory_pool_ = nullptr;
        }

        receiver& operator=(receiver&& other)
        {
            region_ = other.region_;
            pp_ = other.pp_;
            endpoint_ = other.endpoint_;
            memory_pool_ = other.memory_pool_;
            rma_receiver_ = std::move(other.rma_receiver_);
            other.region_ = nullptr;
            other.memory_pool_ = nullptr;

            return *this;
        }

        void handle_recv(fi_addr_t const& src_addr, std::uint64_t len);

        void post_recv();

        libfabric_memory_region* region_;
        parcelport* pp_;
        fid_ep* endpoint_;
        rdma_memory_pool* memory_pool_;
        rma_receiver rma_receiver_;
    };
}}}}

#endif
