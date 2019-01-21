//  Copyright (c) 2019 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <type_traits>
#include <iostream>

namespace hpx { namespace asser {
    struct source_location
    {
        const char* file_name;
        unsigned line_number;
        const char* function_name;
    };

    namespace detail {
        template <typename Expr>
        constexpr auto test_invocable() -> decltype(true ? std::declval<Expr>()() << 1: true)
        {
            return true;
        }

        template <typename Expr>
        constexpr auto test_invocable() -> decltype(true ? std::declval<Expr>()()() : true)
        {
            return false;
        }
    }

    constexpr const char* empty_msg = nullptr;

    // TODO: add attributes to this function to mark that it never returns...
    template <typename Expr, typename Msg> __attribute__((always_inline)) inline
    typename std::enable_if<detail::test_invocable<Expr>()>::type
    evaluate_assert(Expr const& expr, source_location const& loc, const char* expr_string, Msg const& msg)
    {
#if !defined(HPX_DISABLE_ASSERT)
        if (!expr())
        {
            std::cerr << "Assertion failed: " << expr_string << "\n";
            std::cerr << "Function: " << loc.function_name << "\n";
            std::cerr << "File: " << loc.file_name << ":" << loc.line_number << "\n";
            std::abort();
        }
#endif
    }

    template <typename Expr, typename Msg> __attribute__((always_inline)) inline
    typename std::enable_if<!detail::test_invocable<Expr>()>::type
    evaluate_assert(Expr const& expr, source_location const& loc, const char* expr_string, Msg const& msg)
    {
        evaluate_assert(expr(), loc, expr_string, msg);
    }
}}

/// \def HPX_ASSERT(expr, msg)
/// \brief This macro asserts that \a expr evaluates to true.
///
/// \param expr The expression to assert on. This can either be an expression
///             that's convertible to bool or a callable which returns bool
/// \param msg  The optional message that is used to give further information if
///             the assert fails. This can be anything that is streamable to \a
///             std::ostream
///
/// If \a expr
/// evaluates to false, The source location and \a msg is being printed along
/// with the expression and additional, HPX specific system information (see
/// \FIXME). Afterwards the program is being aborted.
///
/// Asserts can be disabled by specifying the macro HPX_DISABLE_ASSERT. This is
/// the default for CMAKE_BUILD_TYPE=Debug
///
/// An abort handler can be added via \a hpx::assert::add_abort_handler
#define HPX_ASSERT_(expr, msg)                                                 \
    ::hpx::asser::evaluate_assert(                                             \
        [&]() { return expr; }, ::hpx::asser::source_location{__FILE__, __LINE__, __func__}, \
        HPX_ASSERT_STRINGIFY(expr), [&](std::ostream& os) -> std::ostream& { if (msg != ::hpx::asser::empty_msg){ os << msg << '\n';} return os;} \
    );                                                                          \
/**/


/// \cond NOINTERNAL
#define HPX_ASSERT_STRINGIFY(text) HPX_ASSERT_STRINGIFY_I(text)
#define HPX_ASSERT_STRINGIFY_I(text) #text

// Helper macros to extract the first and the second macro paramater, the first
// is the expression ...
#define HPX_ASSERT_EXPR(expr, ...) expr
// ... and the second is the msg. msg is optional
#define HPX_ASSERT_MSG(expr, msg, ...) msg

#define HPX_ASSERT(...) \
    HPX_ASSERT_(HPX_ASSERT_EXPR(__VA_ARGS__, ""), HPX_ASSERT_MSG(__VA_ARGS__, ::hpx::asser::empty_msg, "")) \
/**/
