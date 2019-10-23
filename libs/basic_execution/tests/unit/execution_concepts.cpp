//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>

struct no_receiver1
{
    template <typename E>
    void tag_invoke(hpx::execution::detail::set_error_t, E&&)
    {
    }
};

struct no_receiver2
{
    void tag_invoke(hpx::execution::detail::set_done_t) noexcept {}
};

struct proto_receiver
{
    void tag_invoke(hpx::execution::detail::set_done_t) noexcept {}

    template <typename E>
    void tag_invoke(hpx::execution::detail::set_error_t, E&&)
    {
    }
};

struct proto_receiver2
{
    void tag_invoke(hpx::execution::detail::set_done_t) noexcept {}

    template <typename E>
    void tag_invoke(hpx::execution::detail::set_error_t, E&&)
    {
    }
};

struct proto_sender
{
    template <typename Receiver>
    void tag_invoke(hpx::execution::detail::submit_t, Receiver&&)
    {
    }
};

struct proto_sender2
{
};
template <typename Receiver>
void tag_invoke(hpx::execution::detail::submit_t, proto_sender2, Receiver&&)
{
}

struct proto_sender3
{
    void tag_invoke(hpx::execution::detail::submit_t, proto_receiver2) {}
    void tag_invoke(
        hpx::execution::detail::submit_t, hpx::execution::sink_receiver)
    {
    }
};

struct proto_sender4
{
    void tag_invoke(hpx::execution::detail::submit_t, proto_receiver2) {}
};

void tag_invoke(hpx::execution::detail::submit_t, proto_sender4,
    hpx::execution::sink_receiver)
{
}

struct proto_sender5
{
};

void tag_invoke(
    hpx::execution::detail::submit_t, proto_sender5, proto_receiver2)
{
}
void tag_invoke(hpx::execution::detail::submit_t, proto_sender5,
    hpx::execution::sink_receiver)
{
}

struct executor1
{
};

struct executor2
{
};

template <typename... Ts>
void tag_invoke(hpx::execution::detail::execute_t, executor2, Ts&&...)
{
}

struct executor3
{
    template <typename... Ts>
    void tag_invoke(hpx::execution::detail::execute_t, Ts&&...)
    {
    }
};

#include <functional>

struct executor4
{
    void tag_invoke(hpx::execution::detail::execute_t, std::function<void()>) {}
};

void check_concepts()
{
    static_assert(!hpx::execution::is_sender<int>::value, "");
    static_assert(!hpx::execution::is_receiver<int>::value, "");
    static_assert(!hpx::execution::is_scheduler<int>::value, "");
    static_assert(!hpx::execution::is_executor<int>::value, "");

    static_assert(!hpx::execution::is_receiver<no_receiver1>::value, "");
    static_assert(!hpx::execution::is_receiver<no_receiver2>::value, "");
    static_assert(hpx::execution::is_receiver<proto_receiver>::value, "");
    static_assert(hpx::execution::is_receiver<proto_receiver2>::value, "");
    static_assert(hpx::execution::is_sender<proto_sender>::value, "");
    static_assert(hpx::execution::is_sender<proto_sender2>::value, "");
    static_assert(hpx::execution::is_sender<proto_sender3>::value, "");
    static_assert(
        hpx::execution::is_sender_to<proto_sender, proto_receiver>::value, "");
    static_assert(
        hpx::execution::is_sender_to<proto_sender2, proto_receiver>::value, "");
    static_assert(
        !hpx::execution::is_sender_to<proto_sender3, proto_receiver>::value,
        "");
    static_assert(
        hpx::execution::is_sender_to<proto_sender3, proto_receiver2>::value,
        "");
    static_assert(
        !hpx::execution::is_sender_to<proto_sender4, proto_receiver>::value,
        "");
    static_assert(
        hpx::execution::is_sender_to<proto_sender4, proto_receiver2>::value,
        "");
    static_assert(
        !hpx::execution::is_sender_to<proto_sender5, proto_receiver>::value,
        "");
    static_assert(
        hpx::execution::is_sender_to<proto_sender5, proto_receiver2>::value,
        "");

    static_assert(!hpx::execution::is_executor<executor1&>::value, "");
    static_assert(hpx::execution::is_executor<executor2>::value, "");
    static_assert(hpx::execution::is_executor<executor3>::value, "");

    static_assert(hpx::execution::is_executor<executor4>::value, "");
    auto f1 = []() {};
    auto f2 = [](int) {};
    static_assert(
        hpx::execution::is_executor_of<executor2, decltype(f1)>::value, "");
    static_assert(
        !hpx::execution::is_executor_of<executor2, decltype(f2)>::value, "");
    static_assert(
        hpx::execution::is_executor_of<executor2, decltype(f2), int>::value,
        "");
    static_assert(
        hpx::execution::is_executor_of<executor4, decltype(f1)>::value, "");
    static_assert(
        !hpx::execution::is_executor_of<executor4, decltype(f2)>::value, "");
    static_assert(
        !hpx::execution::is_executor_of<executor4, decltype(f2), int>::value,
        "");

    static_assert(
        hpx::execution::is_receiver<hpx::execution::sink_receiver>::value, "");
    static_assert(
        hpx::execution::is_receiver<hpx::execution::shared_state<int>>::value,
        "");
    static_assert(
        hpx::execution::is_receiver<hpx::execution::receiver_reference_wrapper<
            hpx::execution::shared_state<int>>>::value,
        "");

    static_assert(hpx::execution::is_sender_to<proto_sender,
                      hpx::execution::shared_state<int>>::value,
        "");
}

int main()
{
    check_concepts();
}
