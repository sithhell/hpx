//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PROPERTY_HPP
#define HPX_PROPERTY_HPP

// clang-format off
#if defined(DOXYGEN)
namespace hpx {

  // Customization point objects:

  inline namespace unspecified {
    inline constexpr unspecified require_concept = unspecified;
    inline constexpr unspecified require = unspecified;
    inline constexpr unspecified prefer = unspecified;
    inline constexpr unspecified query = unspecified;
  }

  // Property applicability trait:
  template<class T, class P> struct is_applicable_property;

  template<class T, class Property>
    inline constexpr bool is_applicable_property_v = is_applicable_property<T, Property>::value;

  // Customization point type traits:
  template<class T, class P> struct can_require_concept;
  template<class T, class... P> struct can_require;
  template<class T, class... P> struct can_prefer;
  template<class T, class P> struct can_query;

  template<class T, class Property>
    inline constexpr bool can_require_concept_v = can_require_concept<T, Property>::value;
  template<class T, class... Properties>
    inline constexpr bool can_require_v = can_require<T, Properties...>::value;
  template<class T, class... Properties>
    inline constexpr bool can_prefer_v = can_prefer<T, Properties...>::value;
  template<class T, class Property>
    inline constexpr bool can_query_v = can_query<T, Property>::value;

} // namespace hpx
#else
// clang-format on

#include <hpx/properties/require_concept.hpp>
// #include <hpx/properties/require.hpp>
// #include <hpx/properties/prefer.hpp>
// #include <hpx/properties/query.hpp>

#endif
#endif
