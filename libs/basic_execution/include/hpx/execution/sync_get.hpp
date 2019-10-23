//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SYNC_GET_HPP
#define HPX_EXECUTION_SYNC_GET_HPP

#include <hpx/config.hpp>
#include <hpx/assertion.hpp>
#include <hpx/datastructures/optional.hpp>
#include <hpx/execution/blocking.hpp>
#include <hpx/execution/receiver_ref.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/shared_state.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/no_mutex.hpp>
#include <hpx/type_support/unused.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {

    namespace tags {
        struct sync_get_t
        {
            template <typename Sender,
                typename =
                    typename std::enable_if<is_sender<Sender>::value>::type>
            HPX_FORCEINLINE constexpr auto operator()(Sender&& sender) const
                noexcept(is_tag_invocable_nothrow<sync_get_t, Sender>::value)
                    -> hpx::tag_invoke_result_t<sync_get_t, Sender>
            {
                return hpx::tag_invoke(*this, std::forward<Sender>(sender));
            }

            template <typename R, typename Sender>
            HPX_FORCEINLINE R get_blocking(Sender&& sender) const
            {
                shared_state<R, void (*)(), no_mutex> rcv;
                hpx::execution::submit(
                    std::forward<Sender>(sender), receiver_ref(rcv));

                for (std::size_t k = 0; !rcv.is_ready(); ++k)
                {
                    hpx::basic_execution::this_thread::yield_k(
                        k, "hpx::execution::sync_get");
                }

                return rcv.get();
            }

            template <typename Sender,
                typename R = typename sender_value_type<Sender>::type>
            HPX_FORCEINLINE
                typename std::enable_if<!std::is_void<R>::value, R>::type
                operator()(Sender&& sender) const
            {
                blocking_kind block = hpx::execution::blocking(sender);

                if (block == blocking_kind::always_inline)
                {
                    struct
                    {
                        hpx::util::optional<R> r_;
                        void tag_invoke(tags::set_value_t, R&& r)
                        {
                            r_.emplace(std::forward<R>(r));
                        }

                        void tag_invoke(
                            tags::set_error_t, std::exception_ptr ptr)
                        {
                            std::rethrow_exception(ptr);
                        }

                        void tag_invoke(tags::set_done_t) {}
                    } rcv;
                    hpx::execution::submit(
                        std::forward<Sender>(sender), receiver_ref(rcv));
                    return *rcv.r_;
                }

                return get_blocking<R>(std::forward<Sender>(sender));
            }

            template <typename Sender,
                typename R = typename sender_value_type<Sender>::type>
            HPX_FORCEINLINE
                typename std::enable_if<std::is_void<R>::value>::type
                operator()(Sender&& sender) const
            {
                blocking_kind block = hpx::execution::blocking(sender);

                if (block == blocking_kind::always_inline)
                {
                    struct
                    {
                        void tag_invoke(tags::set_value_t) {}

                        void tag_invoke(
                            tags::set_error_t, std::exception_ptr ptr)
                        {
                            std::rethrow_exception(ptr);
                        }

                        void tag_invoke(tags::set_done_t) {}
                    } rcv;
                    hpx::execution::submit(
                        std::forward<Sender>(sender), receiver_ref(rcv));
                    return;
                }

                return get_blocking<R>(std::forward<Sender>(sender));
            }
        };
    }    // namespace tags
    inline constexpr tags::sync_get_t sync_get = {};
}}       // namespace hpx::execution

#endif
