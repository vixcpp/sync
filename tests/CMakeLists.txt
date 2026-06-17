cmake_minimum_required(VERSION 3.20)

include(CTest)

if (BUILD_TESTING)
  enable_testing()
endif()

# --------------------------------------------------------------------
# Target resolution
# --------------------------------------------------------------------
set(VIX_SYNC_TEST_TARGET vix::sync)

if (NOT TARGET ${VIX_SYNC_TEST_TARGET} AND TARGET vix_sync)
  set(VIX_SYNC_TEST_TARGET vix_sync)
endif()

if (NOT TARGET ${VIX_SYNC_TEST_TARGET})
  message(FATAL_ERROR "[sync/tests] Missing sync target (expected vix::sync or vix_sync).")
endif()

# --------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------
function(vix_sync_add_test test_name test_source)
  add_executable(${test_name}
    ${test_source}
  )

  target_compile_features(${test_name}
    PRIVATE
      cxx_std_20
  )

  target_link_libraries(${test_name}
    PRIVATE
      ${VIX_SYNC_TEST_TARGET}
  )

  if (TARGET vix_warnings)
    target_link_libraries(${test_name}
      PRIVATE
        vix_warnings
    )
  endif()

  if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${test_name}
      PRIVATE
        -fno-omit-frame-pointer
        -fsanitize=address,undefined
    )

    target_link_options(${test_name}
      PRIVATE
        -fsanitize=address,undefined
    )
  endif()

  if (BUILD_TESTING)
    add_test(
      NAME ${test_name}
      COMMAND ${test_name}
    )
  endif()
endfunction()

# --------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------
vix_sync_add_test(core_sync_smoke_test
  sync_engine_outbox_smoke_test.cpp
)

vix_sync_add_test(core_sync_permanent_fail_test
  sync_engine_outbox_permanent_fail_test.cpp
)

vix_sync_add_test(core_sync_inflight_timeout_test
  sync_engine_inflight_timeout_test.cpp
)
