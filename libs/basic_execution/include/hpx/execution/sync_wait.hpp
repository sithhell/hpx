//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SYNC_WAIT_HPP
#define HPX_EXECUTION_SYNC_WAIT_HPP

#include <hpx/basic_execution/this_thread.hpp>
#include <hpx/execution/is_ready.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/sender_ref.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/tag_invoke.hpp>

#include <atomic>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    namespace tags {
        struct sync_wait_t
        {
            template <typename Sender1, typename Sender2, typename... Sender>
            constexpr void operator()(
                Sender1&& s1, Sender2&& s2, Sender&&... sender) const
            {
                hpx::tag_invoke(*this, std::forward<Sender1>(s1));
                hpx::tag_invoke(*this, std::forward<Sender2>(s2));
                hpx::tag_invoke(*this, std::forward<Sender>(sender)...);
            }

            template <typename Sender,
                typename =
                    typename std::enable_if<is_sender<Sender>::value>::type>
            constexpr auto operator()(Sender&& sender) const
                noexcept(is_tag_invocable_nothrow<sync_wait_t, Sender>::value)
                    -> hpx::tag_invoke_result_t<sync_wait_t, Sender>
            {
                return hpx::tag_invoke(*this, std::forward<Sender>(sender));
            }

            template <typename Sender,
                typename = typename std::enable_if<is_sender<Sender>::value &&
                    hpx::is_tag_invocable<is_ready_t, Sender>::value>::type>
            constexpr void operator()(Sender&& sender) const
            {
                if (hpx::execution::is_ready(sender))
                    return;

                std::atomic<bool> finished = false;

                hpx::execution::execute(sender_ref(sender), [&finished]() {
                    finished.store(true, std::memory_order_release);
                });

                for (std::size_t k = 0;
                     !finished.load(std::memory_order_acquire); ++k)
                {
                    hpx::basic_execution::this_thread::yield_k(
                        k, "hpx::execution::sync_wait");
                }
            }

            template <typename Sender>
            constexpr typename std::enable_if<is_sender<Sender>::value &&
                !hpx::is_tag_invocable<is_ready_t, Sender>::value>::type
            operator()(Sender&& sender) const
            {
                std::atomic<bool> finished = false;

                hpx::execution::execute(sender_ref(sender), [&finished]() {
                    finished.store(true, std::memory_order_release);
                });

                for (std::size_t k = 0;
                     !finished.load(std::memory_order_acquire); ++k)
                {
                    hpx::basic_execution::this_thread::yield_k(
                        k, "hpx::execution::sync_wait");
                }
            }
        };
    }    // namespace tags
    inline constexpr tags::sync_wait_t sync_wait;
}}       // namespace hpx::execution

#endif
