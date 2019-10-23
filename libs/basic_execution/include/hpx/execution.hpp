//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_HPP
#define HPX_EXECUTION_HPP

// clang-format off
#if defined(DOXYGEN)
namespace hpx { namespace execution {
    // Exception types:

    extern runtime_error const invocation-error;    // exposition only
    struct receiver_invocation_error
      : std::runtime_error
      , nested_exception
    {
        receiver_invocation_error() noexcept
          : runtime_error(invocation-error)
          , nested_exception()
        {
        }
    };

    /// Invocable archetype
    /// The name invocable_archetype is an implementation defined type that models
    /// `invocable`. A program that creates in instance of
    /// `hpx::execution::invocable_archetype` is ill-formed.
    using invocable_archetype = unspecified;

    // Customization points:
    inline namespace unspecified {
        /// The name `hpx::execution::set_value` denotes a customization point
        /// object.
        /// The expression `hpx::execution::set_value(R, Vs...) for some
        /// subexpression R and Vs... is expression-equivalent to:
        ///   * `R.set_value(Vs...)` if that expression is valid.
        ///   * Otherwise, `set_value(R, Vs...)` if that expression is valid.
        ///     The overload is found via ADL and does not include
        ///     `hpx::execution::set_value`
        ///   * If none of the above applies, the expression
        ///     `hpx::execution::set_value(R, Vs...)` leads to a compilation
        ///     error
        /// `R` models `receiver` and needs to accept the values `Vs...` in its
        /// value channel.
        inline constexpr unspecified set_value = unspecified;

        /// The name `hpx::execution::set_done` denotes a customization point
        /// object.
        /// The expression `hpx::execution::set_done(R) for some
        /// subexpression expression-equivalent to:
        ///   * `R.set_done(Vs)` if that expression is valid.
        ///   * Otherwise, `set_done(R)` if that expression is valid.
        ///     The overload is found via ADL and does not include
        ///     `hpx::execution::set_done`
        ///   * If none of the above applies, the expression
        ///     `hpx::execution::set_done(R)` leads to a compilation
        ///     error
        /// `R` models `receiver`.
        inline constexpr unspecified set_done = unspecified;

        inline constexpr unspecified set_error = unspecified;

        inline constexpr unspecified execute = unspecified;

        inline constexpr unspecified submit = unspecified;

        inline constexpr unspecified schedule = unspecified;

        inline constexpr unspecified bulk_execute = unspecified;
    }    // namespace unspecified

    // Concepts:

    template <class T, class E = exception_ptr>
    concept receiver = see-below;

    template <class T, class... An>
    concept receiver_of = see-below;

    template <class S>
    concept sender = see-below;

    template <class S>
    concept typed_sender = see-below;

    template <class S, class R>
    concept sender_to = see-below;

    template <class S>
    concept scheduler = see-below;

    template <class E>
    concept executor = see-below;

    template <class E, class F>
    concept executor_of = see-below;

    // Sender and receiver utilities type
    class sink_receiver;

    template <class S>
    struct sender_traits;

    // Associated execution context property:

    struct context_t;

    constexpr context_t context;

    // Blocking properties:

    struct blocking_t;

    constexpr blocking_t blocking;

    // Properties to allow adaptation of blocking and directionality:

    struct blocking_adaptation_t;

    constexpr blocking_adaptation_t blocking_adaptation;

    // Properties to indicate if submitted tasks represent continuations:

    struct relationship_t;

    constexpr relationship_t relationship;

    // Properties to indicate likely task submission in the future:

    struct outstanding_work_t;

    constexpr outstanding_work_t outstanding_work;

    // Properties for bulk execution guarantees:

    struct bulk_guarantee_t;

    constexpr bulk_guarantee_t bulk_guarantee;

    // Properties for mapping of execution on to threads:

    struct mapping_t;

    constexpr mapping_t mapping;

    // Memory allocation properties:

    template <typename ProtoAllocator>
    struct allocator_t;

    constexpr allocator_t<void> allocator;

    // Executor type traits:

    template <class Executor>
    struct executor_shape;
    template <class Executor>
    struct executor_index;

    template <class Executor>
    using executor_shape_t = typename executor_shape<Executor>::type;
    template <class Executor>
    using executor_index_t = typename executor_index<Executor>::type;

    // Polymorphic executor support:

    class bad_executor;

    template <class... SupportableProperties>
    class any_executor;

    template <class Property>
    struct prefer_only;
}}    // namespace hpx::execution
#else
// clang-format on

// Exception types:
#include <hpx/execution/exceptions.hpp>

// Customization points:
#include <hpx/execution/set_value.hpp>
#include <hpx/execution/set_done.hpp>
#include <hpx/execution/set_error.hpp>
#include <hpx/execution/execute.hpp>
#include <hpx/execution/start.hpp>
#include <hpx/execution/blocking.hpp>
#include <hpx/execution/connect.hpp>
#include <hpx/execution/submit.hpp>
#include <hpx/execution/schedule.hpp>
// #include <hpx/execution/schedule_at.hpp>
// #include <hpx/execution/schedule_after.hpp>
// #include <hpx/execution/schedule_before.hpp>
#include <hpx/execution/bulk_execute.hpp>
// Extensions to P0443:
#include <hpx/execution/is_ready.hpp>
#include <hpx/execution/sync_wait.hpp>
#include <hpx/execution/sync_get.hpp>
// #include <hpx/execution/sequence.hpp>
#include <hpx/execution/when_all.hpp>
// #include <hpx/execution/when_any.hpp>
// #include <hpx/execution/wait_any.hpp>
#include <hpx/execution/async_execute.hpp>
#include <hpx/execution/async_bulk_execute.hpp>
#include <hpx/execution/sync_execute.hpp>
#include <hpx/execution/sync_bulk_execute.hpp>
#include <hpx/execution/then_execute.hpp>
#include <hpx/execution/via.hpp>
// #include <hpx/execution/dataflow.hpp>

// Concepts/Traits:
#include <hpx/execution/receiver.hpp>
#include <hpx/execution/sender.hpp>
#include <hpx/execution/scheduler.hpp>
#include <hpx/execution/executor.hpp>

// Sender and receiver utilities type
#include <hpx/execution/sink_receiver.hpp>
// #include <hpx/execution/sender_traits.hpp>
#include <hpx/execution/receiver_ref.hpp>
#include <hpx/execution/sender_ref.hpp>

// // Non owning, type erased reference wrappers
// #include <hpx/execution/any_executor_ref.hpp>
#include <hpx/execution/any_receiver_ref.hpp>
#include <hpx/execution/any_sender_ref.hpp>

// // Owning, type erased reference wrappers, value semantics
#include <hpx/execution/any_receiver.hpp>
#include <hpx/execution/any_operation.hpp>
#include <hpx/execution/any_sender.hpp>
// #include <hpx/execution/any_executor.hpp>

namespace hpx { namespace execution {
    //     // Associated execution context property:
    //
    //     struct context_t;
    //
    //     constexpr context_t context;
    //
    //     // Properties to allow adaptation of blocking and directionality:
    //
    //     struct blocking_adaptation_t;
    //
    //     constexpr blocking_adaptation_t blocking_adaptation;
    //
    //     // Properties to indicate if submitted tasks represent continuations:
    //
    //     struct relationship_t;
    //
    //     constexpr relationship_t relationship;
    //
    //     // Properties to indicate likely task submission in the future:
    //
    //     struct outstanding_work_t;
    //
    //     constexpr outstanding_work_t outstanding_work;
    //
    //     // Properties for bulk execution guarantees:
    //
    //     struct bulk_guarantee_t;
    //
    //     constexpr bulk_guarantee_t bulk_guarantee;
    //
    //     // Properties for mapping of execution on to threads:
    //
    //     struct mapping_t;
    //
    //     constexpr mapping_t mapping;
    //
    //     // Memory allocation properties:
    //
    //     template <typename ProtoAllocator>
    //     struct allocator_t;
    //
    //     constexpr allocator_t<void> allocator;
    //
    //     // Executor type traits:
    //
    //     template <class Executor>
    //     struct executor_shape;
    //     template <class Executor>
    //     struct executor_index;
    //
    //     template <class Executor>
    //     using executor_shape_t = typename executor_shape<Executor>::type;
    //     template <class Executor>
    //     using executor_index_t = typename executor_index<Executor>::type;
    //
    //     // Polymorphic executor support:
    //
    //     class bad_executor;
    //
    //     template <class... SupportableProperties>
    //     class any_executor;
    //
    //     template <class Property>
    //     struct prefer_only;
}}    // namespace hpx::execution

#endif    // DOXYGEN
#endif
