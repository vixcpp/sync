#  @file CMakeLists.cpp
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira.  All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
#  Vix.cpp
#
# Sync Module
# Purpose:
#   Offline-first synchronization engine for Vix (WAL, outbox, retries,
#   sync workers). Builds as STATIC when sources exist, otherwise as a
#   header-only INTERFACE target.
#
# Public Targets:
#   - vix_sync   : The actual library target (STATIC or INTERFACE)
#   - vix::sync  : Namespaced alias for consumers
#
# Public Dependencies:
#   - vix::utils
#   - vix::net
#
# Options:
#   - VIX_ENABLE_SANITIZERS : Inherit sanitizers from the parent project
#
# Installation/Export:
#   Installs into the umbrella export-set `VixTargets`.
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_sync VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Sources discovery
file(GLOB_RECURSE SYNC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Utils dependency
option(VIX_SYNC_FETCH_UTILS "Auto-fetch vix::utils if missing" ON)

if (NOT TARGET vix::utils)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../utils/CMakeLists.txt")
    message(STATUS "[sync] Adding utils from umbrella: ../utils")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../utils" "utils")
  elseif (VIX_SYNC_FETCH_UTILS)
    include(FetchContent)
    message(STATUS "[sync] Fetching vix::utils via FetchContent")
    FetchContent_Declare(vix_utils
      GIT_REPOSITORY https://github.com/vixcpp/utils.git
      GIT_TAG       dev
    )
    FetchContent_MakeAvailable(vix_utils)
  else()
    message(FATAL_ERROR "vix::utils not found. Enable VIX_SYNC_FETCH_UTILS=ON or provide the target before sync.")
  endif()
endif()

set(VIX_UTILS_TARGET vix::utils)
if (NOT TARGET ${VIX_UTILS_TARGET} AND TARGET vix::utils)
  set(VIX_UTILS_TARGET vix::utils)
endif()

# Net dependency
option(VIX_SYNC_FETCH_NET "Auto-fetch vix::net if missing" ON)

if (NOT TARGET vix::net)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../net/CMakeLists.txt")
    message(STATUS "[sync] Adding net from umbrella: ../net")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../net" "net")
  elseif (VIX_SYNC_FETCH_NET)
    include(FetchContent)
    message(STATUS "[sync] Fetching vix::net via FetchContent")
    FetchContent_Declare(vix_net
      GIT_REPOSITORY https://github.com/vixcpp/net.git
      GIT_TAG        dev
    )
    FetchContent_MakeAvailable(vix_net)
  else()
    message(FATAL_ERROR "vix::net not found. Enable VIX_SYNC_FETCH_NET=ON or provide the target before sync.")
  endif()
endif()

set(VIX_NET_TARGET vix::net)
if (NOT TARGET ${VIX_NET_TARGET} AND TARGET vix::net)
  set(VIX_NET_TARGET vix::net)
endif()

# JSON dependency (needed by FileOutboxStore.cpp via <vix/json/...>)
option(VIX_SYNC_FETCH_JSON "Auto-fetch vix::json if missing" ON)

if (NOT TARGET vix::json AND NOT TARGET vix_json)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../json/CMakeLists.txt")
    message(STATUS "[sync] Adding json from umbrella: ../json")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../json" "json")
  elseif (VIX_SYNC_FETCH_JSON)
    include(FetchContent)
    message(STATUS "[sync] Fetching vix::json via FetchContent")
    FetchContent_Declare(vix_json_mod
      GIT_REPOSITORY https://github.com/vixcpp/json.git
      GIT_TAG        dev
    )
    FetchContent_MakeAvailable(vix_json_mod)
  else()
    message(FATAL_ERROR "vix::json not found. Enable VIX_SYNC_FETCH_JSON=ON or provide the target before sync.")
  endif()
endif()

set(VIX_JSON_TARGET "")
if (TARGET vix::json)
  set(VIX_JSON_TARGET vix::json)
elseif (TARGET vix_json)
  set(VIX_JSON_TARGET vix_json)
endif()

if (VIX_JSON_TARGET STREQUAL "")
  message(FATAL_ERROR "[sync] JSON backend not found (expected vix::json or vix_json)")
endif()

# STATIC
if (SYNC_SOURCES)
  message(STATUS "[sync] Building STATIC library with detected sources.")

  add_library(vix_sync STATIC ${SYNC_SOURCES})

  if (TARGET vix_warnings)
    message(STATUS "[sync] vix_warnings detected (from umbrella)")
    target_link_libraries(vix_sync PUBLIC vix_warnings)
    if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
      target_link_libraries(vix_sync PUBLIC vix_sanitizers)
    endif()

  else()
    message(STATUS "[sync] vix_warnings not found (building sync standalone)")
  endif()

  add_library(vix::sync ALIAS vix_sync)
  target_compile_features(vix_sync PUBLIC cxx_std_20)

  target_include_directories(vix_sync
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_sync
    PUBLIC
      ${VIX_UTILS_TARGET}
      ${VIX_NET_TARGET}
      ${VIX_JSON_TARGET}
  )

  set_target_properties(vix_sync PROPERTIES
    OUTPUT_NAME vix_sync
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME sync
  )

  install(TARGETS vix_sync
    EXPORT VixTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# HEADER-ONLY
else()
  message(STATUS "[sync] Building HEADER-ONLY library (no sources).")

  add_library(vix_sync INTERFACE)
  add_library(vix::sync ALIAS vix_sync)
  target_compile_features(vix_sync INTERFACE cxx_std_20)

  target_include_directories(vix_sync INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_sync INTERFACE
    ${VIX_UTILS_TARGET}
    ${VIX_NET_TARGET}
    ${VIX_JSON_TARGET}
  )


  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_sync INTERFACE vix_sanitizers)
  endif()

  install(TARGETS vix_sync
    EXPORT VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
endif()

# Tests
option(VIX_SYNC_BUILD_TESTS "Build sync module tests" OFF)

if (VIX_SYNC_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# Summary
message(STATUS "------------------------------------------------------")
message(STATUS "vix::sync configured (${PROJECT_VERSION})")
if (SYNC_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "Net target:      ${VIX_NET_TARGET}")
message(STATUS "Utils target:    ${VIX_UTILS_TARGET}")
message(STATUS "Include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include (for <vix/...>)")
message(STATUS "Sanitizers (inherited): ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
