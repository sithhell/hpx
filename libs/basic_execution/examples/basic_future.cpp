//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>

#include <hpx/functional/invoke.hpp>
#include <hpx/functional/unique_function.hpp>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

template <typename T, typename Future>
struct future_base
{
    template <template <typename...> class Tuple,
        template <typename...> class Variant>
    using value_types = Variant<Tuple<T>>;

    template <typename Receiver>
    struct consuming_operation
    {
        using receiver_type = typename std::decay<Receiver>::type;
        Future* this_;
        receiver_type rcv_;

        void tag_invoke(hpx::execution::tags::start_t)
        {
            auto shared_state = this_->shared_state_.get();
            auto handler = [r = std::forward<Receiver>(rcv_),
                               shared_state =
                                   std::move(this_->shared_state_)]() mutable {
                try
                {
                    hpx::execution::set_value(r, shared_state->get());
                }
                catch (...)
                {
                    hpx::execution::set_error(r, std::current_exception());
                }
            };
            {
                if (!shared_state->is_ready())
                {
                    std::unique_lock l(shared_state->mtx_);
                    shared_state->completion_handler_ = std::move(handler);
                    return;
                }
            }
            handler();
        }
    };

    // Value consuming receiver
    template <typename Receiver,
        typename std::enable_if<
            hpx::execution::is_receiver_of<Receiver, T>::value, int>::type = 0>
    consuming_operation<Receiver> tag_invoke(
        hpx::execution::tags::connect_t, Receiver&& r)
    {
        return consuming_operation<Receiver>{
            static_cast<Future*>(this), std::forward<Receiver>(r)};
    }

    template <typename Receiver>
    struct observing_operation
    {
        using receiver_type = typename std::decay<Receiver>::type;
        Future* this_;
        receiver_type rcv_;

        void tag_invoke(hpx::execution::tags::start_t)
        {
            auto shared_state = this_->shared_state_.get();
            auto handler = [r = std::forward<Receiver>(rcv_),
                               shared_state =
                                   std::move(this_->shared_state_)]() mutable {
                try
                {
                    hpx::execution::set_value(r);
                }
                catch (...)
                {
                    hpx::execution::set_error(r, std::current_exception());
                }
            };
            {
                if (!shared_state->is_ready())
                {
                    std::unique_lock l(shared_state->mtx_);
                    shared_state->completion_handler_ = std::move(handler);
                    return;
                }
            }
            handler();
        }
    };

    // Observing observing
    template <typename Receiver,
        typename std::enable_if<
            !hpx::execution::is_receiver_of<Receiver, T>::value, int>::type = 0>
    observing_operation<Receiver> tag_invoke(
        hpx::execution::tags::connect_t, Receiver&& r)
    {
        return observing_operation<Receiver>{
            static_cast<Future*>(this), std::forward<Receiver>(r)};
    }

    T get()
    {
        auto this_ = static_cast<Future*>(this);
        auto shared_state = this_->shared_state_.get();
        hpx::execution::sync_wait(*this_);

        return shared_state->get();
    }
};

template <typename Future>
struct future_base<void, Future>
{
    template <template <typename...> class Tuple,
        template <typename...> class Variant>
    using value_types = Variant<Tuple<>>;

    template <typename Receiver>
    struct operation
    {
        using receiver_type = typename std::decay<Receiver>::type;
        Future* this_;
        receiver_type rcv_;

        void tag_invoke(hpx::execution::tags::start_t)
        {
            auto shared_state = this_->shared_state_.get();
            auto handler = [r = std::move(rcv_)]() mutable {
                try
                {
                    hpx::execution::set_value(r);
                }
                catch (...)
                {
                    hpx::execution::set_error(r, std::current_exception());
                }
            };
            {
                if (!shared_state->is_ready())
                {
                    std::unique_lock l(shared_state->mtx_);
                    shared_state->completion_handler_ = std::move(handler);
                    return;
                }
            }
            handler();
        }
    };

    template <typename Receiver>
    operation<Receiver> tag_invoke(
        hpx::execution::tags::connect_t, Receiver&& r)
    {
        return operation<Receiver>{
            static_cast<Future*>(this), std::forward<Receiver>(r)};
    }

    void get()
    {
        auto this_ = static_cast<Future*>(this);
        auto shared_state = this_->shared_state_.get();
        hpx::execution::sync_wait(*this_);

        shared_state->get();
        this_->shared_state_.reset();

        return;
    }
};

template <typename T>
struct future : future_base<T, future<T>>
{
    template <template <typename...> class Tuple,
        template <typename...> class Variant>
    using value_types = typename future_base<T,
        future<T>>::template value_types<Tuple, Variant>;

    template <template <typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    template <typename Sender>
    static constexpr bool can_construct_from_sender(
        std::false_type /*is_same<future>*/)
    {
        return hpx::execution::is_sender_to<Sender,
            hpx::execution::shared_state<T>>::value;
    }

    template <typename Sender>
    static constexpr bool can_construct_from_sender(
        std::true_type /*is_same<future>*/)
    {
        return false;
    }

    template <typename Sender,
        typename = typename std::enable_if<can_construct_from_sender<Sender>(
            std::is_same<future, typename std::decay<Sender>::type>{})>::type>
    future(Sender sender)
      : shared_state_(new hpx::execution::shared_state<T>())
    {
        hpx::execution::submit(
            std::move(sender), hpx::execution::receiver_ref(*shared_state_));
    }

    bool is_ready() const
    {
        return shared_state_->is_ready();
    }

    void wait()
    {
        hpx::execution::sync_wait(*this);
    }

    friend bool tag_invoke(
        hpx::tag_t<hpx::execution::is_ready>, future const& f)
    {
        return f.is_ready();
    }

    using future_base<T, future<T>>::get;

    std::unique_ptr<hpx::execution::shared_state<T>> shared_state_;
};

struct inline_executor
{
    template <typename... Ts>
    friend void tag_invoke(
        hpx::tag_t<hpx::execution::execute>, inline_executor, Ts&&... ts)
    {
        hpx::util::invoke(std::forward<Ts>(ts)...);
    }
};

struct thread_executor
{
    template <typename... Ts>
    friend void tag_invoke(
        hpx::tag_t<hpx::execution::execute>, thread_executor, Ts&&... ts)
    {
        std::thread t(hpx::util::deferred_call(std::forward<Ts>(ts)...));
        t.detach();
    }
};

struct inline_scheduler
{
    struct scheduler_sender
    {
        template <template <typename...> class Tuple,
            template <typename...> class Variant>
        using value_types = Variant<Tuple<>>;

        template <template <typename...> class Variant>
        using error_types = Variant<>;

        template <typename Receiver>
        struct operation
        {
            using receiver_type = typename std::decay<Receiver>::type;
            receiver_type rcv_;
            void tag_invoke(hpx::execution::tags::start_t) noexcept
            {
                hpx::execution::set_value(std::forward<Receiver>(rcv_));
            }
        };
        template <typename Receiver>
        operation<Receiver> tag_invoke(
            hpx::execution::tags::connect_t, Receiver&& r)
        {
            return operation<Receiver>{std::forward<Receiver>(r)};
        }
    };

    scheduler_sender tag_invoke(hpx::execution::tags::schedule_t)
    {
        return {};
    }
};

int main()
{
    inline_executor exec;
    static_assert(hpx::execution::is_executor<inline_executor>::value, "");
    static_assert(hpx::execution::is_executor<thread_executor>::value, "");
    static_assert(hpx::execution::is_executor<inline_executor&>::value, "");
    static_assert(hpx::execution::is_executor<thread_executor&>::value, "");
    int i = 0;
    hpx::execution::execute(exec, [&]() { ++i; });
    std::cout << i << '\n';
    hpx::execution::execute(
        exec, [&](int j) { i += j; }, 5);
    std::cout << i << '\n';
    hpx::execution::execute(exec, [&]() { ++i; });
    std::cout << i << '\n';

    auto t = hpx::execution::async_execute(exec, []() {
        std::cout << "Hello world!\n";
        return 42;
    });

    static_assert(hpx::execution::is_sender<decltype(t)>::value, "");

    future<int> f(std::move(t));

    std::cout << f.get() << '\n';

    thread_executor exec2;
    future<int> f2 = hpx::execution::async_execute(exec2, []() {
        std::cout << "Hello world!\n";
        return 42;
    });

    std::cout << hpx::execution::sync_get(f2) << '\n';

    future<void> f3 = hpx::execution::async_execute(
        exec2, []() { std::cout << "Hello world1!\n"; });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    hpx::execution::sync_wait(f3);

    static_assert(hpx::execution::is_sender<future<int>>::value, "");
    static_assert(hpx::execution::is_sender_to<future<void>,
                      hpx::execution::shared_state<void>>::value,
        "");
    static_assert(hpx::execution::is_sender_to<future<int>,
                      hpx::execution::shared_state<void>>::value,
        "");
    static_assert(hpx::execution::is_sender_to<future<int>,
                      hpx::execution::shared_state<int>>::value,
        "");
    static_assert(hpx::execution::is_sender_to<decltype(t),
                      hpx::execution::shared_state<void>>::value,
        "");
    static_assert(!hpx::execution::is_sender_to<decltype(t),
                      hpx::execution::shared_state<std::string>>::value,
        "");
    // This does not compile, as expected
    //future<std::string> t1 = hpx::execution::async_execute(exec, []() { return 5; });
    future<void> t1 = hpx::execution::async_execute(exec, []() { return 5; });

    static_assert(hpx::execution::is_sender<future<void>>::value, "");
    static_assert(hpx::execution::is_sender<future<int>>::value, "");

    hpx::execution::sync_get(t1);

    {
        auto s = hpx::execution::then_execute(
            hpx::execution::async_execute(exec, []() { return 7; }),
            [](int i) { return 5 + i; });
        std::cout << hpx::execution::sync_get(std::move(s)) << '\n';
    }
    {
        auto t1 = hpx::execution::async_execute(exec, []() { return 7; });
        auto t2 = hpx::execution::then_execute(
            std::move(t1), [](int i) { return i * 8; });
        auto s = hpx::execution::then_execute(
            std::move(t2), [](int i) { return 5 + i; });
        std::cout << typeid(s).name() << '\n';
        future<int> f = std::move(s);
        std::cout << hpx::execution::sync_get(f) << '\n';
    }
    {
        auto t1 = hpx::execution::async_execute(exec, []() { return 7; });
        future<int> t2 = hpx::execution::then_execute(
            std::move(t1), [](int i) { return i * 8; });
        auto s = hpx::execution::then_execute(
            std::move(t2), [](int i) { return 5 + i; });
        std::cout << typeid(s).name() << '\n';
        future<int> f = std::move(s);
        std::cout << hpx::execution::sync_get(f) << '\n';
    }

    {
        auto s = hpx::execution::then_execute(
            hpx::execution::async_execute(
                exec, []() { std::cout << "no result!\n"; }),
            []() { return 5; });
        std::cout << hpx::execution::sync_get(std::move(s)) << '\n';
    }
    {
        auto t = hpx::execution::async_execute(
            exec, []() { std::cout << "no result!\n"; });
        auto s = hpx::execution::then_execute(std::move(t), []() { return 5; });
        auto s2 = hpx::execution::then_execute(
            hpx::execution::via(std::move(s), exec2),
            [](int i) { return 5 + i; });
        std::cout << hpx::execution::sync_get(std::move(s2)) << '\n';
    }
    {
        inline_scheduler scheduler;
        hpx::execution::any_sender<void()> t = hpx::execution::async_execute(
            exec, []() { std::cout << "no result!\n"; });
        auto s = hpx::execution::then_execute(std::move(t), []() { return 5; });
        auto s2 = hpx::execution::then_execute(
            hpx::execution::via(std::move(s), scheduler),
            [](int i) { return 5 + i; });
        std::cout << hpx::execution::sync_get(std::move(s2)) << '\n';
    }
    {
        auto t1 = hpx::execution::when_all(
            hpx::execution::async_execute(exec, []() { return 1; }),
            hpx::execution::async_execute(exec, []() { return 2; }),
            hpx::execution::async_execute(exec, []() { return 3; }),
            hpx::execution::async_execute(exec, []() { return 4; }));
        auto t2 = hpx::execution::then_execute(
            std::move(t1), [](int i1, int i2, int i3, int i4) {
                std::cout << "1: " << i1 << '\n';
                std::cout << "2: " << i2 << '\n';
                std::cout << "3: " << i3 << '\n';
                std::cout << "4: " << i4 << '\n';
            });
        hpx::execution::sync_get(std::move(t2));
    }
    {
        auto t1 = hpx::execution::when_all(
            hpx::execution::async_execute(exec, []() { return 1; }),
            hpx::execution::async_execute(exec, []() { return 2; }),
            hpx::execution::async_execute(
                exec, []() { std::cout << "None!\n"; }),
            hpx::execution::async_execute(exec, []() { return 4; }));
        auto t2 = hpx::execution::then_execute(
            std::move(t1), [](int i1, int i2, hpx::util::tuple<>, int i4) {
                std::cout << "1: " << i1 << '\n';
                std::cout << "2: " << i2 << '\n';
                std::cout << "4: " << i4 << '\n';
            });
        hpx::execution::sync_get(std::move(t2));
    }

    std::cout << hpx::execution::sync_execute(exec, []() { return 42; })
              << '\n';

    hpx::execution::bulk_execute(
        exec, 10, [](std::size_t i) { std::cout << i << '\n'; });

    hpx::execution::sync_get(hpx::execution::async_bulk_execute(
        exec, 10, [](std::size_t i) { std::cout << "async: " << i << '\n'; }));

    hpx::execution::any_receiver<void(int)> rcv(
        hpx::execution::shared_state<int>{});

    hpx::execution::set_value(rcv, 5);
}
