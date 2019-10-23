//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_SHARED_STATE_HPP
#define HPX_EXECUTION_SHARED_STATE_HPP

#include <hpx/config.hpp>
#include <hpx/assertion.hpp>
#include <hpx/datastructures/optional.hpp>
#include <hpx/execution/tags.hpp>
#include <hpx/functional/unique_function.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx { namespace execution {
    enum class sender_state
    {
        empty,
        value,
        exception
    };

    template <typename T, typename SharedState>
    struct shared_state_base
    {
        using value_type = T;

        using storage_type =
            typename std::aligned_storage<(sizeof(value_type) >
                                              sizeof(std::exception_ptr)) ?
                    sizeof(value_type) :
                    sizeof(std::exception_ptr),
                (alignof(value_type) > alignof(std::exception_ptr)) ?
                    alignof(value_type) :
                    alignof(std::exception_ptr)>::type;

        HPX_FORCEINLINE void tag_invoke(tags::set_value_t, T t)
        {
            using completion_handler_type =
                typename SharedState::completion_handler_type;
            auto this_ = static_cast<SharedState*>(this);
            completion_handler_type completion_handler;
            {
                std::unique_lock l(this_->mtx_);
                completion_handler = std::move(this_->completion_handler_);
            }

            new (&this_->storage_) T(std::move(t));
            this_->state_.store(sender_state::value, std::memory_order_release);

            if (completion_handler)
                completion_handler();
        }

        HPX_FORCEINLINE T get()
        {
            auto this_ = static_cast<SharedState*>(this);
            HPX_ASSERT(this_->state_.load(std::memory_order_acquire) !=
                sender_state::empty);
            if (this_->state_.load(std::memory_order_acquire) ==
                sender_state::value)
            {
                return std::move(*reinterpret_cast<T*>(&this_->storage_));
            }

            std::rethrow_exception(std::move(
                *reinterpret_cast<std::exception_ptr*>(&this_->storage_)));
        }
    };

    template <typename SharedState>
    struct shared_state_base<void, SharedState>
    {
        using value_type = void;

        using storage_type =
            typename std::aligned_storage<sizeof(std::exception_ptr),
                alignof(std::exception_ptr)>::type;

        template <typename... Ts>
        HPX_FORCEINLINE void tag_invoke(tags::set_value_t, Ts&&...)
        {
            using completion_handler_type =
                typename SharedState::completion_handler_type;
            completion_handler_type completion_handler;
            auto this_ = static_cast<SharedState*>(this);
            this_->state_.store(sender_state::value, std::memory_order_release);
            {
                std::unique_lock l(this_->mtx_);
                completion_handler = std::move(this_->completion_handler_);
            }
            if (completion_handler)
                completion_handler();
        }

        HPX_FORCEINLINE void get()
        {
            auto this_ = static_cast<SharedState*>(this);
            HPX_ASSERT(this_->state_.load(std::memory_order_acquire) !=
                sender_state::empty);
            if (this_->state_.load(std::memory_order_acquire) ==
                sender_state::value)
            {
                return;
            }

            std::rethrow_exception(std::move(
                *reinterpret_cast<std::exception_ptr*>(&this_->storage_)));
        }
    };

    template <typename T,
        typename CompletionHandler = hpx::util::unique_function_nonser<void()>,
        typename Mutex = std::mutex>
    struct shared_state
      : shared_state_base<T, shared_state<T, CompletionHandler, Mutex>>
    {
        using completion_handler_type = CompletionHandler;
        using mutex_type = Mutex;

        shared_state()
          : state_(sender_state::empty)
          , completion_handler_(nullptr)
        {
        }

        template <typename Ptr>
        void destruct(Ptr* ptr)
        {
            ptr->~Ptr();
        }
        void destruct(void*) noexcept {}

        HPX_FORCEINLINE ~shared_state()
        {
            auto state = state_.load(std::memory_order_acquire);
            void* storage = &storage_;
            if (state == sender_state::value)
            {
                destruct(static_cast<value_type*>(storage));
                return;
            }
            if (state == sender_state::exception)
            {
                destruct(static_cast<std::exception_ptr*>(storage));
                return;
            }
        }

        shared_state(shared_state&& other) noexcept
          : state_(sender_state::empty)
          , completion_handler_(nullptr)
        {
            std::swap(storage_, other.storage_);
            state_.store(other.state_.load());
            {
                std::unique_lock l(other.mtx_);
                std::swap(completion_handler_, other.completion_handler_);
            }
        }

        using value_type = typename shared_state_base<T,
            shared_state<T, CompletionHandler, Mutex>>::value_type;
        using storage_type = typename shared_state_base<T,
            shared_state<T, CompletionHandler, Mutex>>::storage_type;

        using shared_state_base<T,
            shared_state<T, CompletionHandler, Mutex>>::tag_invoke;
        using shared_state_base<T,
            shared_state<T, CompletionHandler, Mutex>>::get;

        void tag_invoke(tags::set_error_t, std::exception_ptr ptr)
        {
            completion_handler_type completion_handler;
            {
                std::unique_lock l(mtx_);
                completion_handler = std::move(completion_handler_);
            }

            new (&storage_) std::exception_ptr(std::move(ptr));
            state_.store(sender_state::exception, std::memory_order_release);
            if (completion_handler)
                completion_handler();
        }

        void tag_invoke(tags::set_done_t) noexcept {}

        HPX_FORCEINLINE bool is_ready() const
        {
            return state_.load(std::memory_order_acquire) !=
                sender_state::empty;
        }

        mutable mutex_type mtx_;
        std::atomic<sender_state> state_;
        storage_type storage_;
        completion_handler_type completion_handler_;
    };
}}    // namespace hpx::execution

#endif
