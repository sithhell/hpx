//  Copyright (c) 2016 John Biddiscombe
//  Copyright (c) 2017 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LIBFABRIC_CONTROLLER_HPP
#define HPX_PARCELSET_POLICIES_LIBFABRIC_CONTROLLER_HPP

// config
#include <hpx/config/defines.hpp>
//
#include <hpx/runtime/parcelset/parcelport_impl.hpp>
#include <hpx/runtime/agas/addressing_service.hpp>

#include <hpx/util/command_line_handling.hpp>

//
#include <plugins/parcelport/parcelport_logging.hpp>
#include <plugins/parcelport/libfabric/rdma_locks.hpp>
#include <plugins/parcelport/libfabric/receiver.hpp>
#include <plugins/parcelport/libfabric/sender.hpp>
#include <plugins/parcelport/libfabric/rma_receiver.hpp>
#include <plugins/parcelport/libfabric/locality.hpp>
//
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
#include <array>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdint>
#include <cstddef>
#include <cstring>
//
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
#include "fabric_error.hpp"

#ifdef HPX_PARCELPORT_LIBFABRIC_GNI
# include "rdma/fi_ext_gni.h"
#endif


namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{

    class libfabric_controller
    {
    public:
        typedef hpx::lcos::local::spinlock mutex_type;
        typedef hpx::parcelset::policies::libfabric::unique_lock<mutex_type> unique_lock;
        typedef hpx::parcelset::policies::libfabric::scoped_lock<mutex_type> scoped_lock;

        std::unordered_map<uint32_t, fi_addr_t> endpoint_av_;

        locality here_;
        locality agas_;

        struct fi_info    *fabric_info_;
        struct fid_fabric *fabric_;
        struct fid_domain *fabric_domain_;
        // Server/Listener for RDMA connections.
        struct fid_ep     *endpoint_;
        std::vector<struct fid_ep*> tx_endpoint_;
        std::vector<struct fid_ep*> rx_endpoint_;

        // we will use just one event queue for all connections
        struct fid_cq     *txcq_, *rxcq_;
        struct fid_av     *av_;

        bool immediate_;

        // --------------------------------------------------------------------
        // constructor gets info from device and sets up all necessary
        // maps, queues and server endpoint etc
        libfabric_controller(
            std::string provider,
            std::string domain)
          : fabric_info_(nullptr)
          , fabric_(nullptr)
          , fabric_domain_(nullptr)
          , endpoint_(nullptr)
          , tx_endpoint_(19, nullptr)
          , rx_endpoint_(19, nullptr)
            //
          , txcq_(nullptr)
          , rxcq_(nullptr)
          , av_(nullptr)
            //
          , immediate_(false)
        {
            FUNC_START_DEBUG_MSG;
            open_fabric(provider, domain);

            // Create a memory pool for pinned buffers
            memory_pool_.reset(
                new rma_memory_pool<libfabric_region_provider>(fabric_domain_));

            // setup address vector
            std::uint32_t N = hpx::get_config().get_num_localities();
            std::cerr << "Number of localities: " << N << '\n';
            fi_av_attr av_attr = {};
            if (fabric_info_->ep_attr->type == FI_EP_RDM || fabric_info_->ep_attr->type == FI_EP_DGRAM) {
//                 if (fabric_info_->domain_attr->av_type != FI_AV_UNSPEC)
//                     av_attr.type = fabric_info_->domain_attr->av_type;
//                 else {
//                     LOG_DEBUG_MSG("Setting map type to FI_AV_MAP");
                    av_attr.type  = FI_AV_TABLE;
//                 }
                av_attr.count = 20;
                av_attr.ep_per_node = 2;

                av_attr.rx_ctx_bits = 12;

                LOG_DEBUG_MSG("Creating address vector ");
                std::cerr << "creating av...\n";
                int ret = fi_av_open(fabric_domain_, &av_attr, &av_, nullptr);
                if (ret) throw fabric_error(ret, "fi_av_open");
            }

            // setup a passive listener, or an active RDM endpoint
            here_ = create_local_endpoint();
            std::cout << here_ << '\n';

            LOG_DEBUG_MSG("Calling boot PMI");
            boot_PMI();
            FUNC_END_DEBUG_MSG;
        }

        void boot_PMI();

        // --------------------------------------------------------------------
        // clean up all resources
        ~libfabric_controller()
        {
            unsigned int messages_handled = 0;
            unsigned int acks_received    = 0;
            unsigned int msg_plain        = 0;
            unsigned int msg_rma          = 0;
            unsigned int sent_ack         = 0;
            unsigned int rma_reads        = 0;
            unsigned int recv_deletes     = 0;
            //
            for (auto &r : receivers_) {
                r.cleanup();
                // from receiver
                messages_handled += r.messages_handled_;
                acks_received    += r.acks_received_;
                // from rma_receivers
                msg_plain        += r.msg_plain_;
                msg_rma          += r.msg_rma_;
                sent_ack         += r.sent_ack_;
                rma_reads        += r.rma_reads_;
                recv_deletes     += r.recv_deletes_;
            }

            LOG_DEBUG_MSG(
                   "Received messages " << decnumber(messages_handled)
                << "Received acks "     << decnumber(acks_received)
                << "Sent acks "     << decnumber(sent_ack)
                << "Total reads "       << decnumber(rma_reads)
                << "Total deletes "     << decnumber(recv_deletes)
                << "deletes error " << decnumber(messages_handled - recv_deletes));

            // Cleaning up receivers to avoid memory leak errors.
            receivers_.clear();

            LOG_DEBUG_MSG("closing fabric_->fid");
            if (fabric_)
                fi_close(&fabric_->fid);
            LOG_DEBUG_MSG("closing endpoint_->fid");
            for (auto & ep : tx_endpoint_)
            {
                if (ep)
                    fi_close(&ep->fid);
            }

            for (auto & ep : rx_endpoint_)
            {
                if (ep)
                    fi_close(&ep->fid);
            }

            if (endpoint_)
                fi_close(&endpoint_->fid);
            LOG_DEBUG_MSG("closing fabric_domain_->fid");
            if (fabric_domain_)
                fi_close(&fabric_domain_->fid);
            // clean up
            LOG_DEBUG_MSG("freeing fabric_info");
            fi_freeinfo(fabric_info_);
        }

        // --------------------------------------------------------------------
        // initialize the basic fabric/domain/name
        void open_fabric(
            std::string provider, std::string domain)
        {
            FUNC_START_DEBUG_MSG;
            struct fi_info *fabric_hints_ = fi_allocinfo();
            if (!fabric_hints_) {
                throw fabric_error(-1, "Failed to allocate fabric hints");
            }
            // we require message and RMA support, so ask for them
            // we also want receives to carry source address info
            fabric_hints_->caps                   = FI_MSG | FI_RMA | FI_SOURCE |
                FI_WRITE | FI_READ | FI_REMOTE_READ | FI_REMOTE_WRITE | FI_RMA_EVENT;// | FI_NAMED_RX_CTX;
            fabric_hints_->mode                   = FI_CONTEXT | FI_LOCAL_MR;
            fabric_hints_->fabric_attr->prov_name = strdup(provider.c_str());
            LOG_DEBUG_MSG("fabric provider " << fabric_hints_->fabric_attr->prov_name);
            if (domain.size()>0) {
                fabric_hints_->domain_attr->name  = strdup(domain.c_str());
                LOG_DEBUG_MSG("fabric domain "   << fabric_hints_->domain_attr->name);
            }

            // use infiniband type basic registration for now
            fabric_hints_->domain_attr->mr_mode = FI_MR_BASIC;

            // Disable the use of progress threads
            fabric_hints_->domain_attr->control_progress = FI_PROGRESS_MANUAL;
            fabric_hints_->domain_attr->data_progress = FI_PROGRESS_MANUAL;

            // Enable thread safe mode Does not work with psm2 provider
#if !defined(HPX_PARCELPORT_LIBFABRIC_PSM2)
            fabric_hints_->domain_attr->threading = FI_THREAD_SAFE;
            fabric_hints_->addr_format = FI_ADDR_PSMX;
#endif

            // Enable resource management
            fabric_hints_->domain_attr->resource_mgmt = FI_RM_ENABLED;

            LOG_DEBUG_MSG("Selecting endpoint type RDM");
            fabric_hints_->ep_attr->type = FI_EP_RDM;
#if defined(HPX_PARCELPORT_LIBFABRIC_PSM2)
            // enable scalable EPs by setting the counts greater than 1...
            fabric_hints_->ep_attr->tx_ctx_cnt = 2;
#endif

            // by default, we will always want completions on both tx/rx events
            fabric_hints_->tx_attr->op_flags = FI_COMPLETION;
            fabric_hints_->rx_attr->op_flags = FI_COMPLETION;

            uint64_t flags = 0;
            LOG_DEBUG_MSG("Getting initial info about fabric");
            int ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION,FI_MINOR_VERSION), nullptr, nullptr,
                flags, fabric_hints_, &fabric_info_);
            if (ret) {
                throw fabric_error(ret, "Failed to get fabric info");
            }
            LOG_DEBUG_MSG("Fabric info " << fi_tostr(fabric_info_, FI_TYPE_INFO));

            immediate_ = (fabric_info_->rx_attr->mode & FI_RX_CQ_DATA)!=0;
            LOG_DEBUG_MSG("Fabric supports immediate data " << immediate_);

            LOG_DEBUG_MSG("Creating fabric object");
            ret = fi_fabric(fabric_info_->fabric_attr, &fabric_, nullptr);
            if (ret) {
                throw fabric_error(ret, "Failed to get fi_fabric");
            }

            // Allocate a domain.
            LOG_DEBUG_MSG("Allocating domain ");
            ret = fi_domain(fabric_, fabric_info_, &fabric_domain_, nullptr);
            if (ret) throw fabric_error(ret, "fi_domain");

            // Cray specific. Disable memory registration cache
            _set_disable_registration();

            fi_freeinfo(fabric_hints_);
            FUNC_END_DEBUG_MSG;
        }

        // -------------------------------------------------------------------
        // create endpoint and get ready for possible communications
        void startup(parcelport *pp)
        {
            FUNC_START_DEBUG_MSG;
            //
//             bind_endpoint_to_queues();

            // filling our vector of receivers...
            std::size_t num_receivers = HPX_PARCELPORT_LIBFABRIC_MAX_PREPOSTS;
            receivers_.reserve(num_receivers);
            for(std::size_t i = 0; i != num_receivers; ++i)
            {
                receivers_.emplace_back(pp, tx_endpoint_[0], rx_endpoint_[0], *memory_pool_);
            }
        }

        // --------------------------------------------------------------------
        // Special GNI extensions to disable memory registration cache

        // this helper function only works for string ops
        void _set_check_domain_op_value(int op, const char *value)
        {
#ifdef HPX_PARCELPORT_LIBFABRIC_GNI
            int ret;
            struct fi_gni_ops_domain *gni_domain_ops;
            char *get_val;

            ret = fi_open_ops(&fabric_domain_->fid, FI_GNI_DOMAIN_OPS_1,
                      0, (void **) &gni_domain_ops, nullptr);
            if (ret) throw fabric_error(ret, "fi_open_ops");
            LOG_DEBUG_MSG("domain ops returned " << hexpointer(gni_domain_ops));

            ret = gni_domain_ops->set_val(&fabric_domain_->fid,
                    (dom_ops_val_t)(op), &value);
            if (ret) throw fabric_error(ret, "set val (ops)");

            ret = gni_domain_ops->get_val(&fabric_domain_->fid,
                    (dom_ops_val_t)(op), &get_val);
            LOG_DEBUG_MSG("Cache mode set to " << get_val);
            if (std::string(value) != std::string(get_val))
                throw fabric_error(ret, "get val");
#endif
        }

        void _set_disable_registration()
        {
#ifdef HPX_PARCELPORT_LIBFABRIC_GNI
            _set_check_domain_op_value(GNI_MR_CACHE, "none");
#endif
        }

        // --------------------------------------------------------------------
        locality create_local_endpoint()
        {
            struct fid *id;
            LOG_DEBUG_MSG("Creating active endpoint");
            if (endpoint_ == nullptr)
            {
                new_endpoint_active();
                // make sure address vector is created
                create_completion_queues(fabric_info_);
                bind_endpoint_to_queues();
            }
            LOG_DEBUG_MSG("active endpoint " << hexpointer(endpoint_));
            id = &endpoint_->fid;
            locality::locality_data local_addr = {{0}};
            std::size_t addrlen = locality::array_size;
            LOG_DEBUG_MSG("Fetching local address using size " << decnumber(addrlen));
            int ret = fi_getname(id, local_addr.data(), &addrlen);
            if (ret || (addrlen>locality::array_size)) {
                fabric_error(ret, "fi_getname - size error or other problem");
            }

            std::cerr << addrlen << '\n';

//             LOG_EXCLUSIVE(
            {
                std::stringstream temp1;
                for (std::size_t i=0; i<locality::array_length; ++i) {
                    temp1 << ipaddress(local_addr[i]);
                }
                std::cerr << "address info is " << temp1.str().c_str() << '\n';
                std::stringstream temp2;
                for (std::size_t i=0; i<locality::array_length; ++i) {
                    temp2 << hexuint32(local_addr[i]);
                }
                std::cerr << "address info is " << temp2.str().c_str() << '\n';
            }//);
            std::cerr << "created address " << hexuint32(locality(local_addr).ip_address()) << '\n';
            FUNC_END_DEBUG_MSG;
            return locality(local_addr);
        }

        // --------------------------------------------------------------------
        void new_endpoint_active()
        {
            FUNC_START_DEBUG_MSG;
            // create an 'active' endpoint that can be used for sending/receiving
            LOG_DEBUG_MSG("Creating active scalable endpoint");

            std::cout << "Rx contexts: " << fabric_info_->ep_attr->rx_ctx_cnt << '\n';
            std::cout << "Tx contexts: " << fabric_info_->ep_attr->tx_ctx_cnt << '\n';
            fabric_info_->ep_attr->tx_ctx_cnt = 1;
            fabric_info_->ep_attr->rx_ctx_cnt = 1;
            int ret = fi_scalable_ep(fabric_domain_, fabric_info_, &endpoint_, nullptr);

//             LOG_DEBUG_MSG("Got info mode " << (info->mode & FI_NOTIFY_FLAGS_ONLY));
            if (ret) throw fabric_error(ret, "fi_endpoint");

            if (av_) {
                std::cerr << "Binding endpoint to AV" << '\n';
                ret = fi_scalable_ep_bind(endpoint_, &av_->fid, 0);
                if (ret) throw fabric_error(ret, "bind endpoint");
            }

            for (std::size_t i = 0; i < 1; ++i)
            {
                ret = fi_tx_context(endpoint_, i, nullptr, &tx_endpoint_[i], nullptr);
                if (ret) throw fabric_error(ret, "fi_tx_context");
                ret = fi_rx_context(endpoint_, i, nullptr, &rx_endpoint_[i], nullptr);
                if (ret) throw fabric_error(ret, "fi_rx_context");
            }

        }

        // --------------------------------------------------------------------
        void bind_endpoint_to_queues()
        {
            int ret = 0;

            if (txcq_) {
                LOG_DEBUG_MSG("Binding endpoint to TX CQ");
                std::cerr << "Binding endpoint to TX CQ" << '\n';

                for (std::size_t i = 0; i < 1; ++i)
                {
                    ret = fi_ep_bind(tx_endpoint_[i], &txcq_->fid, FI_SEND);
                    if (ret) throw fabric_error(ret, "fi_ep_bind txcq");

                    ret = fi_enable(tx_endpoint_[i]);
                    if (ret) throw fabric_error(ret, "fi_enable rx_endpoint");
                }
            }

            if (rxcq_) {
                LOG_DEBUG_MSG("Binding endpoint to RX CQ");
                std::cerr << "Binding endpoint to RX CQ" << '\n';
                for (std::size_t i = 0; i < 1; ++i)
                {
                    ret = fi_ep_bind(rx_endpoint_[i], &rxcq_->fid, FI_RECV);
                    if (ret) throw fabric_error(ret, "fi_ep_bind rxcq");

                    ret = fi_enable(rx_endpoint_[i]);
                    if (ret) throw fabric_error(ret, "fi_enable rx_endpoint");
                }
            }

//             LOG_DEBUG_MSG("Enabling endpoint " << hexpointer(endpoint));
            ret = fi_enable(endpoint_);
            if (ret) throw fabric_error(ret, "fi_enable");

            FUNC_END_DEBUG_MSG;
        }

        // --------------------------------------------------------------------
        void initialize_localities(hpx::agas::addressing_service &as)
        {
            FUNC_START_DEBUG_MSG;
#ifndef HPX_PARCELPORT_LIBFABRIC_HAVE_PMI
            std::cerr << "initialize localities...\n";
            std::uint32_t N = hpx::get_config().get_num_localities();
            LOG_DEBUG_MSG("Parcelport initialize_localities with " << N << " localities");

            for (std::uint32_t i=0; i<N; ++i) {
                hpx::naming::gid_type l = hpx::naming::get_gid_from_locality_id(i);
                LOG_DEBUG_MSG("Resolving locality" << l);
                // each locality may be reachable by mutiplte parcelports
                const parcelset::endpoints_type &res = as.resolve_locality(l);
                // get the fabric related data
                auto it = res.find("libfabric");
                LOG_DEBUG_MSG("locality resolution " << it->first << " => " <<it->second);
                const hpx::parcelset::locality &fabric_locality = it->second;
                const locality &loc = fabric_locality.get<locality>();
                // put the provide specific data into the address vector
                // so that we can look it  up later
                /*fi_addr_t dummy =*/ insert_address(loc);
            }
#endif
            LOG_DEBUG_MSG("Done getting localities ");
            FUNC_END_DEBUG_MSG;
        }

        // --------------------------------------------------------------------
        fi_addr_t get_fabric_address(const locality &dest_fabric)
        {
            fi_addr_t fi_addr = endpoint_av_.find(dest_fabric.ip_address())->second;
            std::cerr << "found address " << hexuint32(dest_fabric.ip_address()) << " " << hexpointer(fi_addr) << '\n';
            fi_addr_t add = fi_rx_addr(fi_addr, 0, 12);
            std::cerr << "found address idx " << hexuint32(dest_fabric.ip_address()) << " " <<  hexpointer(add) << '\n';
            return fi_rx_addr(fi_addr, 0, 12);
        }

        // --------------------------------------------------------------------
        const locality & here() const { return here_; }

        // --------------------------------------------------------------------
        const bool & immedate_data_supported() const { return immediate_; }

        // --------------------------------------------------------------------
        // returns true when all connections have been disconnected and none are active
        bool isTerminated() {
            return false;
            //return (qp_endpoint_map_.size() == 0);
        }

        // --------------------------------------------------------------------
        // This is the main polling function that checks for work completions
        // and connection manager events, if stopped is true, then completions
        // are thrown away, otherwise the completion callback is triggered
        int poll_endpoints(bool stopped=false)
        {
            int work = poll_for_work_completions();

            return work;
        }

        // --------------------------------------------------------------------
        int poll_for_work_completions()
        {
            // @TODO, disable polling until queues are initialized to avoid this check
            // if queues are not setup, don't poll
            if (HPX_UNLIKELY(!rxcq_)) return 0;
            //
            return poll_send_queue() + poll_recv_queue();
        }

        // --------------------------------------------------------------------
        int poll_send_queue();
        int poll_recv_queue();
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        inline struct fid_domain * get_domain() {
            return fabric_domain_;
        }

        // --------------------------------------------------------------------
        inline rma_memory_pool<libfabric_region_provider>& get_memory_pool() {
            return *memory_pool_;
        }

        // --------------------------------------------------------------------
        void create_completion_queues(struct fi_info *info)
        {
            FUNC_START_DEBUG_MSG;

            // only one thread must be allowed to create queues,
            // and it is only required once
            scoped_lock lock(initialization_mutex_);
            if (txcq_!=nullptr || rxcq_!=nullptr) {
                return;
            }

            int ret;

            fi_cq_attr cq_attr = {};
            // @TODO - why do we check this
//             if (cq_attr.format == FI_CQ_FORMAT_UNSPEC) {
                LOG_DEBUG_MSG("Setting CQ attribute to FI_CQ_FORMAT_MSG");
                cq_attr.format = FI_CQ_FORMAT_MSG;
//             }

            // open completion queue on fabric domain and set context ptr to tx queue
            cq_attr.wait_obj = FI_WAIT_NONE;
            cq_attr.size = info->tx_attr->size;
            info->tx_attr->op_flags |= FI_COMPLETION;
            cq_attr.flags = 0;//|= FI_COMPLETION;
            LOG_DEBUG_MSG("Creating CQ with tx size " << decnumber(info->tx_attr->size));
            ret = fi_cq_open(fabric_domain_, &cq_attr, &txcq_, &txcq_);
            if (ret) throw fabric_error(ret, "fi_cq_open");

            // open completion queue on fabric domain and set context ptr to rx queue
            cq_attr.size = info->rx_attr->size;
            LOG_DEBUG_MSG("Creating CQ with rx size " << decnumber(info->rx_attr->size));
            ret = fi_cq_open(fabric_domain_, &cq_attr, &rxcq_, &rxcq_);
            if (ret) throw fabric_error(ret, "fi_cq_open");

            FUNC_END_DEBUG_MSG;
        }

        // --------------------------------------------------------------------
        fi_addr_t insert_address(const locality &remote)
        {
            FUNC_START_DEBUG_MSG;
            LOG_DEBUG_MSG("inserting address in vector "
                << ipaddress(remote.ip_address()));
            fi_addr_t result = 0x0;
//             LOG_EXCLUSIVE(
            {
                std::stringstream temp1;
                for (std::size_t i=0; i<locality::array_length; ++i) {
                    temp1 << ipaddress(((uint32_t const*)remote.fabric_data())[i]);
                }
                std::cerr << "remote address info is " << temp1.str().c_str() << '\n';
                std::stringstream temp2;
                for (std::size_t i=0; i<locality::array_length; ++i) {
                    temp2 << hexuint32(((uint32_t const*)remote.fabric_data())[i]);
                }
                std::cerr << "remote address info is " << temp2.str().c_str() << '\n';
            }//);
            int ret = fi_av_insert(av_, remote.fabric_data(), 1, &result, 0, nullptr);
            if (ret < 0) {
                fabric_error(ret, "fi_av_insert");
            }
            else if (ret != 1) {
                fabric_error(ret, "fi_av_insert did not return 1");
            }
            std::cerr << "inserted address " << hexuint32(remote.ip_address()) << hexpointer(result) << '\n';
            endpoint_av_.insert(std::make_pair(remote.ip_address(), result));
            LOG_DEBUG_MSG("Address inserted in vector "
                << ipaddress(remote.ip_address()) << hexuint64(result));
            FUNC_END_DEBUG_MSG;
            return result;
        }

    private:
        // Pinned memory pool used for allocating buffers
        std::unique_ptr<rma_memory_pool<libfabric_region_provider>>  memory_pool_;

        // Shared completion queue for all endoints
        // Count outstanding receives posted to SRQ + Completion queue
        std::vector<receiver> receivers_;

        // only allow one thread to handle connect/disconnect events etc
        mutex_type            initialization_mutex_;
        mutex_type            endpoint_map_mutex_;
        mutex_type            polling_mutex_;

        // used to skip polling event channel too frequently
        typedef std::chrono::time_point<std::chrono::system_clock> time_type;
        time_type event_check_time_;
        uint32_t  event_pause_;

    };

}}}}

#endif
