cmake_minimum_required(VERSION 3.20)

# Enable CTest
include(CTest)

if (BUILD_TESTING)
  enable_testing()
endif()

# ----------------------------
# Common test settings
# ----------------------------
set(VIX_SYNC_TEST_TARGET vix::sync)
if (NOT TARGET ${VIX_SYNC_TEST_TARGET} AND TARGET vix_sync)
  set(VIX_SYNC_TEST_TARGET vix_sync)
endif()

if (NOT TARGET ${VIX_SYNC_TEST_TARGET})
  message(FATAL_ERROR "[sync/tests] Missing sync target (expected vix::sync or vix_sync).")
endif()

# ----------------------------
# Sync / Outbox smoke test
# ----------------------------
add_executable(core_sync_smoke_test
  sync_engine_outbox_smoke_test.cpp
)

target_link_libraries(core_sync_smoke_test PRIVATE
  ${VIX_SYNC_TEST_TARGET}
)

target_compile_features(core_sync_smoke_test PRIVATE cxx_std_20)

if (BUILD_TESTING)
  add_test(
    NAME core_sync_smoke_test
    COMMAND core_sync_smoke_test
  )
endif()

# ----------------------------
# Sync / Outbox permanent failure test (F1)
# ----------------------------
add_executable(core_sync_permanent_fail_test
  sync_engine_outbox_permanent_fail_test.cpp
)

target_link_libraries(core_sync_permanent_fail_test PRIVATE
  ${VIX_SYNC_TEST_TARGET}
)

target_compile_features(core_sync_permanent_fail_test PRIVATE cxx_std_20)

if (BUILD_TESTING)
  add_test(
    NAME core_sync_permanent_fail_test
    COMMAND core_sync_permanent_fail_test
  )
endif()

# ----------------------------
# Sync / Outbox in-flight timeout test
# ----------------------------
add_executable(core_sync_inflight_timeout_test
  sync_engine_inflight_timeout_test.cpp
)

target_link_libraries(core_sync_inflight_timeout_test PRIVATE
  ${VIX_SYNC_TEST_TARGET}
)

target_compile_features(core_sync_inflight_timeout_test PRIVATE cxx_std_20)

if (BUILD_TESTING)
  add_test(
    NAME core_sync_inflight_timeout_test
    COMMAND core_sync_inflight_timeout_test
  )
endif()
