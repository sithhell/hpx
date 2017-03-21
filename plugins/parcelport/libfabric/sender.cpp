//  Copyright (c) 2015-2017 John Biddiscombe
//  Copyright (c) 2017      Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <plugins/parcelport/libfabric/libfabric_memory_region.hpp>
#include <plugins/parcelport/libfabric/rdma_memory_pool.hpp>
#include <plugins/parcelport/libfabric/pinned_memory_vector.hpp>
#include <plugins/parcelport/libfabric/header.hpp>
#include <plugins/parcelport/libfabric/sender.hpp>

#include <hpx/util/atomic_count.hpp>
#include <hpx/util/unique_function.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <rdma/fi_endpoint.h>
// include for iovec
#include <sys/uio.h>

#include <memory>

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    void sender::async_write_impl()
    {
        // for each zerocopy chunk, we must create a memory region for the data
        LOG_EXCLUSIVE(
            for (auto &c : buffer.chunks_) {
            LOG_DEBUG_MSG("write : chunk : size " << hexnumber(c.size_)
                    << " type " << decnumber((uint64_t)c.type_)
                    << " rkey " << hexpointer(c.rkey_)
                    << " cpos " << hexpointer(c.data_.cpos_)
                    << " pos " << hexpointer(c.data_.pos_)
                    << " index " << decnumber(c.data_.index_));
        });

        rma_regions_.reserve(buffer_.chunks_.size());

        // for each zerocopy chunk, we must create a memory region for the data
        int index = 0;
        for (auto &c : buffer_.chunks_)
        {
            if (c.type_ == serialization::chunk_type_pointer)
            {
//                     send_data.has_zero_copy  = true;
                // if the data chunk fits into a memory block, copy it
                LOG_EXCLUSIVE(util::high_resolution_timer regtimer);
                libfabric_memory_region *zero_copy_region;
                if (c.size_<=HPX_PARCELPORT_LIBFABRIC_MEMORY_COPY_THRESHOLD) {
                    zero_copy_region = memory_pool_->allocate_region((std::max)
                      (c.size_, (std::size_t)RDMA_POOL_SMALL_CHUNK_SIZE));
                    char *zero_copy_memory =
                        (char*)(zero_copy_region->get_address());
                    std::memcpy(zero_copy_memory, c.data_.cpos_, c.size_);
                    // the pointer in the chunk info must be changed
                    buffer_.chunks_[index] = serialization::create_pointer_chunk(
                        zero_copy_memory, c.size_);
                    LOG_DEBUG_MSG("Time to copy memory (ns) "
                        << decnumber(regtimer.elapsed_nanoseconds()));
                }
                else {
                    // create a memory region from the pointer
                    zero_copy_region = new libfabric_memory_region(
                            domain_, c.data_.cpos_, (std::max)(c.size_,
                                (std::size_t)RDMA_POOL_SMALL_CHUNK_SIZE));
                    LOG_DEBUG_MSG("Time to register memory (ns) "
                        << decnumber(regtimer.elapsed_nanoseconds()));
                }
                c.rkey_  = zero_copy_region->get_remote_key();
                LOG_DEVEL_MSG("Zero-copy rdma Get region "
                    << decnumber(index) << " created for address "
                    << hexpointer(zero_copy_region->get_address())
                    << " and rkey " << hexpointer(c.rkey_));
                rma_regions_.push_back(zero_copy_region);
            }
            index++;
        }

        // Increase the completion count to be equal to two
        completion_count_ += 2;
        HPX_ASSERT(completion_count_ == 2);

        char *header_memory = (char*)(header_region_->get_address());

        // create the header in the pinned memory block
        LOG_DEBUG_MSG("Placement new for header with piggyback copy disabled");
        header_ = new(header_memory) header_type(buffer_, this);
        header_region_->set_message_length(header_->header_length());

        LOG_DEVEL_MSG("sending, buffsize "
            << decnumber(header_->size())
            << "header_length " << decnumber(header_->header_length())
            << "chunks zerocopy( " << decnumber(header_->num_chunks().first) << ") "
            << ", chunk_flag " << decnumber(header_->header_length())
            << ", normal( " << decnumber(header_->num_chunks().second) << ") "
            << ", chunk_flag " << decnumber(header_->header_length())
            << "tag " << hexuint64(header_->tag())
        );

        // Get the block of pinned memory where the message was encoded
        // during serialization
        message_region_ = buffer_.data_.m_region_;
        message_region_->set_message_length(header_->size());
        LOG_DEBUG_MSG("Found region allocated during encode_parcel : address "
            << hexpointer(buffer_.data_.m_array_)
            << " region "<< hexpointer(message_region_));

        // header region is always sent, message region is usually piggybacked
        int num_regions = 1;

        struct iovec region_list[2] = {
            { header_region_->get_address(),
                header_region_->get_message_length() },
            { message_region_->get_address(),
                message_region_->get_message_length() } };

        void *desc[2] = {
            header_region_->get_desc(),
            message_region_->get_desc() };

        if (header_->chunk_data()) {
            LOG_DEBUG_MSG("Chunk info is piggybacked");
        }
        else {
            throw std::runtime_error("@TODO : implement chunk info rdma get "
                "when zero-copy chunks exceed header space");
        }

        if (header_->piggy_back()) {
            LOG_DEBUG_MSG("Main message is piggybacked");
            num_regions += 1;
        }
        else {
            LOG_DEBUG_MSG("Main message NOT piggybacked ");
            header_->set_message_rdma_size(header_->size());
            header_->set_message_rdma_key(message_region_->get_remote_key());
            header_->set_message_rdma_addr(message_region_->get_address());

            LOG_DEBUG_MSG("RDMA message " << hexnumber(buffer_.data_.size())
                << "desc " << hexnumber(message_region_->get_desc())
                << "pos " << hexpointer(message_region_->get_address()));
        }

        // send the header/main_chunk to the destination,
        // wr_id is header_region (entry 0 in region_list)
        LOG_DEVEL_MSG("fi_send num regions " << decnumber(num_regions)
            << " client " << hexpointer(endpoint_)
            << " fi_addr " << hexpointer(dst_addr_)
            << " header_region"
            << " buffer " << hexpointer(header_region_->get_address())
            << " region " << hexpointer(header_region_));

        fi_msg msg;
        msg.msg_iov = region_list;
        msg.desc = desc;
        msg.iov_count = num_regions;
        msg.addr = dst_addr_;
        msg.context = this;
        msg.data = 0;

        int ret = 0;
        for (std::size_t k = 0; true; ++k)
        {
            ret = fi_sendmsg(endpoint_, &msg, 0);
            if (ret == -FI_EAGAIN)
            {
                std::cerr << "reposting send...\n";
                hpx::util::detail::yield_k(k,
                    "libfabric::sender::async_write");
                continue;
            }
            if (ret) throw fabric_error(ret, "fi_sendv");
            break;
        }

        // log the time spent in performance counter
//                buffer.data_point_.time_ =
//                        timer.elapsed_nanoseconds() - buffer.data_point_.time_;

        // parcels_posted_.add_data(buffer.data_point_);
        FUNC_END_DEBUG_MSG;
    }

    void sender::handle_send_completion()
    {
        LOG_DEVEL_MSG("sender handle send_completion message("
            << hexpointer(header_->piggy_back()) << ") RMA regions:" << rma_regions_.size());
        if (header_->piggy_back() && rma_regions_.empty())
        {
            error_code ec;
            handler_(ec);
        }

        cleanup();
    }

    void sender::handle_message_completion()
    {
        LOG_DEVEL_MSG("sender handle message_completion message("
            << hexpointer(header_->piggy_back()) << ") RMA regions:" << rma_regions_.size());
        if (!header_->piggy_back() || !rma_regions_.empty())
        {
            error_code ec;
            handler_(ec);
        }
        cleanup();
    }

    void sender::cleanup()
    {
        if (--completion_count_ > 0)
            return;

        memory_pool_->deallocate(message_region_);

        for (auto& region: rma_regions_)
        {
            memory_pool_->deallocate(region);
        }
        rma_regions_.clear();
        header_ = nullptr;
        postprocess_handler_(this);
    }

}}}}
