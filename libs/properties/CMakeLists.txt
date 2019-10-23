# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(properties_headers
  hpx/property.hpp
)

set(properties_compat_headers)

set(properties_sources)

include(HPX_AddModule)
add_hpx_module(properties
  COMPATIBILITY_HEADERS OFF
  DEPRECATION_WARNINGS
  FORCE_LINKING_GEN
  GLOBAL_HEADER_GEN OFF
  SOURCES ${properties_sources}
  HEADERS ${properties_headers}
  COMPAT_HEADERS ${properties_compat_headers}
  DEPENDENCIES
  CMAKE_SUBDIRS examples tests
)
