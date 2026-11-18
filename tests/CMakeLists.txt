if (NOVA_BUILD_TESTS)
    include(FetchContent)
    if (NOVA_FETCH_GTEST)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    enable_testing()

    add_executable(nova_core_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/logger_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/config_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/error_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/init_test.cpp
    )
    target_link_libraries(nova_core_tests PRIVATE nova_core GTest::gtest_main)
    add_test(NAME nova_core_tests COMMAND nova_core_tests)

    add_subdirectory(src/loader/pe)
    add_subdirectory(src/loader/module)
    add_subdirectory(src/loader/loader)
    add_subdirectory(src/runtime)
    add_subdirectory(src/sdk)
endif()
