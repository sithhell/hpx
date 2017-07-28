//  Copyright (c) 2016 John Biddiscombe
//  Copyright (c) 2017 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <plugins/parcelport/libfabric/context.hpp>
#include <plugins/parcelport/libfabric/libfabric_controller.hpp>
#include <plugins/parcelport/libfabric/receiver.hpp>
#include <plugins/parcelport/libfabric/sender.hpp>
#include <plugins/parcelport/libfabric/rma_receiver.hpp>

#ifdef HPX_PARCELPORT_LIBFABRIC_HAVE_PMI
//
# include <pmi2.h>
//
# include <boost/archive/iterators/base64_from_binary.hpp>
# include <boost/archive/iterators/binary_from_base64.hpp>
# include <boost/archive/iterators/transform_width.hpp>

using namespace boost::archive::iterators;

typedef
    base64_from_binary<
        transform_width<std::string::const_iterator, 6, 8>
    > base64_t;

typedef
    transform_width<
        binary_from_base64<std::string::const_iterator>, 8, 6
    > binary_t;
#endif

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    void libfabric_controller::boot_PMI()
    {
#ifdef HPX_PARCELPORT_LIBFABRIC_HAVE_PMI
        int spawned;
        int size;
        int rank;
        int appnum;

        LOG_DEBUG_MSG("Calling PMI init");
        PMI2_Init(&spawned, &size, &rank, &appnum);
        LOG_DEBUG_MSG("Called PMI init on rank" << decnumber(rank));

        // create address vector and queues we need if bootstrapping
        create_completion_queues(fabric_info_, size);

        // we must pass out libfabric data to other nodes
        // encode it as a string to put into the PMI KV store
        std::string encoded_locality(
            base64_t((const char*)(here_.fabric_data())),
            base64_t((const char*)(here_.fabric_data()) + locality::array_size));
        int encoded_length = encoded_locality.size();
        LOG_DEBUG_MSG("Encoded locality as " << encoded_locality
            << " with length " << decnumber(encoded_length));

        // Key name for PMI
        std::string pmi_key = "hpx_libfabric_" + std::to_string(rank);
        // insert out data in the KV store
        LOG_DEBUG_MSG("Calling PMI2_KVS_Put on rank " << decnumber(rank));
        PMI2_KVS_Put(pmi_key.data(), encoded_locality.data());

        // Wait for all to do the same
        LOG_DEBUG_MSG("Calling PMI2_KVS_Fence on rank " << decnumber(rank));
        PMI2_KVS_Fence();

        // read libfabric data for all nodes and insert into our Address vector
        for (int i = 0; i < size; ++i)
        {
            // read one locality key
            std::string pmi_key = "hpx_libfabric_" + std::to_string(i);
            char encoded_data[locality::array_size*2];
            int length = 0;
            PMI2_KVS_Get(0, i, pmi_key.data(), encoded_data,
                encoded_length + 1, &length);
            if (length != encoded_length)
            {
                LOG_ERROR_MSG("PMI value length mismatch, expected "
                    << decnumber(encoded_length) << "got " << decnumber(length));
            }

            // decode the string back to raw locality data
            LOG_DEBUG_MSG("Calling decode for " << decnumber(i)
                << " locality data on rank " << decnumber(rank));
            locality new_locality;
            std::copy(binary_t(encoded_data),
                      binary_t(encoded_data + encoded_length),
                      (new_locality.fabric_data_writable()));

            // insert locality into address vector
            LOG_DEBUG_MSG("Calling insert_address for " << decnumber(i)
                << "on rank " << decnumber(rank));
            insert_address(new_locality);
            LOG_DEBUG_MSG("rank " << decnumber(i)
                << "added to address vector");
            if (i == 0)
            {
                agas_ = new_locality;
            }
        }

        PMI2_Finalize();
#endif
    }

    int libfabric_controller::poll_send_queue()
    {
        LOG_TIMED_INIT(poll);
        LOG_TIMED_BLOCK(poll, DEVEL, 5.0, { LOG_DEBUG_MSG("poll_send_queue"); });

        fi_cq_msg_entry entry;
        int ret = 0;
        {
            std::unique_lock<mutex_type> l(polling_mutex_, std::try_to_lock);
            if (l)
                ret = fi_cq_read(txcq_, &entry, 1);
        }
        if (ret>0) {
            LOG_DEBUG_MSG("Completion txcq wr_id "
                << fi_tostr(&entry.flags, FI_TYPE_OP_FLAGS)
                << " (" << decnumber(entry.flags) << ") "
                << "context " << hexpointer(entry.op_context)
                << "length " << hexuint32(entry.len));
            if (entry.flags & FI_RMA) {
                LOG_DEBUG_MSG("Received a txcq RMA completion "
                    << "Context " << hexpointer(entry.op_context));
                rma_base *base = reinterpret_cast<context<rma_base>*>(entry.op_context)->this_;
                HPX_ASSERT(dynamic_cast<rma_receiver *>(base));
                static_cast<rma_receiver *>(base)->handle_rma_read_completion();
            }
            else if (entry.flags == (FI_MSG | FI_SEND)) {
                LOG_DEBUG_MSG("Received a txcq RMA send completion");
                rma_base *base = reinterpret_cast<context<rma_base>*>(entry.op_context)->this_;
                HPX_ASSERT(dynamic_cast<sender *>(base));
                static_cast<sender *>(base)->handle_send_completion();
            }
            else {
                LOG_DEBUG_MSG("$$$$$ Received an unknown txcq completion ***** "
                    << decnumber(entry.flags));
                std::terminate();
            }
            return 1;
        }
        else if (ret==0 || ret==-FI_EAGAIN) {
            // do nothing, we will try again on the next check
            LOG_TIMED_MSG(poll, DEVEL, 10, "txcq FI_EAGAIN");
        }
        else if (ret == -FI_EAVAIL) {
            struct fi_cq_err_entry e = {};
            int err_sz = fi_cq_readerr(txcq_, &e ,0);
            (void)err_sz;
            // flags might not be set correctly
            if (e.flags == (FI_MSG | FI_SEND)) {
                LOG_ERROR_MSG("txcq Error for FI_SEND with len " << hexlength(e.len)
                    << "context " << hexpointer(e.op_context));
            }
            if (e.flags & FI_RMA) {
                LOG_ERROR_MSG("txcq Error for FI_RMA with len " << hexlength(e.len)
                    << "context " << hexpointer(e.op_context));
            }
            (*reinterpret_cast<context<rma_base>*>(entry.op_context))->handle_error(e);
        }
        else {
            LOG_ERROR_MSG("unknown error in completion txcq read");
        }
        return 0;
    }

    // --------------------------------------------------------------------
    int libfabric_controller::poll_recv_queue()
    {
        LOG_TIMED_INIT(poll);
        LOG_TIMED_BLOCK(poll, DEVEL, 5.0, { LOG_DEBUG_MSG("poll_recv_queue"); });

        fi_addr_t src_addr = 0;
        fi_cq_msg_entry entry;

        // receives will use fi_cq_readfrom as we want the source address
        int ret = 0;
        {
            std::unique_lock<mutex_type> l(polling_mutex_, std::try_to_lock);
            if (l)
            {
//                 hpx::util::attach_debugger();
                ret = fi_cq_readfrom(rxcq_, &entry, 1, &src_addr);
            }
        }
        if (ret>0) {
//             std::cerr << "Completion rxcq wr_id "
//                 << fi_tostr(&entry.flags, FI_TYPE_OP_FLAGS)
//                 << " (" << decnumber(entry.flags) << ") "
//                 << "source " << hexpointer(src_addr)
//                 << "context " << hexpointer(entry.op_context)
//                 << "length " << hexuint32(entry.len) << '\n';
            if (src_addr == FI_ADDR_NOTAVAIL)
            {
                LOG_DEBUG_MSG("Source address not available...\n");
                std::terminate();
            }
//                     if ((entry.flags & FI_RMA) == FI_RMA) {
//                         LOG_DEBUG_MSG("Received an rxcq RMA completion");
//                     }
            else if (entry.flags == (FI_MSG | FI_RECV)) {
                LOG_DEBUG_MSG("Received an rxcq recv completion "
                    << hexpointer(entry.op_context));

                (*reinterpret_cast<context<receiver>*>(entry.op_context))->
                    handle_recv(src_addr, entry.len);
                return 1;
            }
            else {
                LOG_DEBUG_MSG("Received an unknown rxcq completion "
                    << decnumber(entry.flags));
                std::terminate();
            }
        }
        else if (ret==0 || ret==-FI_EAGAIN) {
            // do nothing, we will try again on the next check
            LOG_TIMED_MSG(poll, DEVEL, 10, "rxcq FI_EAGAIN");
        }
        else if (ret == -FI_EAVAIL) {
            struct fi_cq_err_entry e = {};
            int err_ret = fi_cq_readerr(rxcq_, &e ,0);
            if (err_ret > 0)
            {
                std::cerr << "    " << fi_strerror(e.err) << '\n';
//             char ebuf[1024] = {0};
//             const char* fi_err = fi_cq_strerror(rxcq_, e.prov_errno, e.err_data, ebuf, 1024);
//             LOG_ERROR_MSG("rxcq Error with flags " << hexlength(e.flags)
//                 << "len " << hexlength(e.len) << fi_err << ' ' << ebuf);
            }
            else
            {
                std::cerr << "    " << fi_strerror(-err_ret) << '\n';
            }
            if (src_addr == FI_ADDR_NOTAVAIL)
            {
                LOG_DEBUG_MSG("Source address not available...\n");
            }
            else
            {
                std::uint32_t tmp[2] = {0};
                std::memcpy(tmp, &src_addr, sizeof(tmp));
                std::cerr << "Source address " << hexuint32(tmp[0]) << hexuint32(tmp[1]) << '\n';
            }
        }
        else {
            LOG_ERROR_MSG("unknown error in completion rxcq read");
        }
        return 0;
    }


}}}}
