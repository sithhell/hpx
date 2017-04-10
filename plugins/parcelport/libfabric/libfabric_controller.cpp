//  Copyright (c) 2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// config
#include <hpx/config/defines.hpp>
//
#include <hpx/lcos/local/shared_mutex.hpp>
#include <hpx/lcos/promise.hpp>
#include <hpx/lcos/future.hpp>
//
#include <hpx/runtime/parcelset/parcelport_impl.hpp>
#include <hpx/runtime/agas/addressing_service.hpp>
//
#include <plugins/parcelport/parcelport_logging.hpp>
#include <plugins/parcelport/libfabric/rdma_locks.hpp>
#include <plugins/parcelport/libfabric/libfabric_controller.hpp>
#include <plugins/parcelport/libfabric/parcelport.hpp>
#include <plugins/parcelport/libfabric/receiver.hpp>
#include <plugins/parcelport/libfabric/sender.hpp>
#include <plugins/parcelport/libfabric/rma_receiver.hpp>
#include <plugins/parcelport/libfabric/locality.hpp>
//
#include <plugins/parcelport/unordered_map.hpp>
//
#include <memory>
#include <deque>
#include <chrono>
#include <iostream>
#include <functional>
#include <map>
#include <atomic>
#include <string>
#include <utility>
#include <cstdint>
//
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
#include "fabric_error.hpp"

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    int libfabric_controller::poll_for_work_completions(bool stopped)
    {
        // @TODO, disable polling until queues are initialized to avoid this check
        // if queues are not setup, don't poll
        if (HPX_UNLIKELY(!rxcq_)) return 0;

        int result = 0;

        LOG_TIMED_INIT(poll);
        LOG_TIMED_BLOCK(poll, DEVEL, 5.0,
            {
                LOG_DEVEL_MSG("Polling work completion channel");
            }
        )
/*
struct fi_cq_msg_entry {
void     *op_context; // operation context
uint64_t flags;       // completion flags
size_t   len;         // size of received data
};
*/
        //std::array<char, 256> buffer;
        fi_addr_t src_addr;
        fi_cq_msg_entry entry;
        int ret = 0;
        {
            std::unique_lock<parcelport::mutex_type> l(pp_->fi_mutex_, std::try_to_lock);
            if (l)
                ret = fi_cq_read(txcq_, &entry, 1);
        }
        if (ret>0) {
//                 hpx::util::unlock_guard_try<unique_lock> ul(lock);
            //struct fi_cq_msg_entry *entry = (struct fi_cq_msg_entry *)(buffer.data());
            LOG_DEVEL_MSG("Completion txcq wr_id "
                << fi_tostr(&entry.flags, FI_TYPE_OP_FLAGS) << " (" << decnumber(entry.flags) << ") "
                << hexpointer(entry.op_context) << "length " << hexuint32(entry.len));
            if (entry.flags & FI_RMA) {
                LOG_DEBUG_MSG("Received a txcq RMA completion");
                rma_receiver* rcv = reinterpret_cast<rma_receiver*>(entry.op_context);
                rcv->handle_read_completion();
            }
            else if (entry.flags == (FI_MSG | FI_SEND)) {
                LOG_DEBUG_MSG("Received a txcq RMA send completion");
                sender* handler = reinterpret_cast<sender*>(entry.op_context);
                handler->handle_send_completion();
            }
            else {
                LOG_DEVEL_MSG("$$$$$ Received an unknown txcq completion ***** "
                    << decnumber(entry.flags));
//                     std::terminate();
            }
            result = 1;
        }
        else if (ret==0 || ret==-EAGAIN) {
            result = 0;
            // do nothing, we will try again on the next check
            LOG_TIMED_MSG(poll, DEVEL, 5, "txcq EAGAIN");
        }
        else if (ret<0) {
            struct fi_cq_err_entry e = {};
            int err_sz = 0;
            err_sz = fi_cq_readerr(txcq_, &e ,0);
            LOG_ERROR_MSG("txcq Error with flags " << e.flags << " len " << e.len);
            throw fabric_error(ret, "completion txcq read");
        }

//             if (!lock) return 0;

        // receives will use fi_cq_readfrom as we want the source address
        ret = 0;
        {
            std::unique_lock<parcelport::mutex_type> l(pp_->fi_mutex_, std::try_to_lock);
            if (l)
                ret = fi_cq_readfrom(rxcq_, &entry, 1, &src_addr);
        }
        if (ret>0) {
            LOG_DEVEL_MSG("Completion rxcq wr_id "
                << fi_tostr(&entry.flags, FI_TYPE_OP_FLAGS) << " (" << decnumber(entry.flags) << ") "
                << " source " << hexpointer(src_addr)
                << "context " << hexpointer(entry.op_context)
                << "length " << hexuint32(entry.len));
//                void *client = reinterpret_cast<libfabric_memory_region*>
//                    (entry.op_context)->get_user_data();
            if (src_addr == FI_ADDR_NOTAVAIL)
            {
                LOG_DEVEL_MSG("Source address not available...\n");
                std::terminate();
            }
//                     if ((entry.flags & FI_RMA) == FI_RMA) {
//                         LOG_DEVEL_MSG("Received an rxcq RMA completion");
//                     }
            else if (entry.flags == (FI_MSG | FI_RECV)) {
                LOG_DEVEL_MSG("Received an rxcq recv completion" << entry.op_context);
                reinterpret_cast<receiver *>(entry.op_context)->handle_recv(src_addr, entry.len);
            }
            else {
                LOG_DEVEL_MSG("Received an unknown rxcq completion "
                    << decnumber(entry.flags));
                std::terminate();
            }
            result = 1;
        }
        else if (ret==0 || ret==-EAGAIN) {
            if (result != 1)
                result = 0;
            // do nothing, we will try again on the next check
            LOG_TIMED_MSG(poll, DEVEL, 5, "rxcq EAGAIN");
        }
        else if (ret<0) {
            struct fi_cq_err_entry e = {};
            int err_sz = 0;
            err_sz = fi_cq_readerr(rxcq_, &e ,0);
            LOG_ERROR_MSG("rxcq Error with flags " << e.flags << " len " << e.len);
            throw fabric_error(ret, "completion rxcq read");
        }

        return result;
    }
}}}}
