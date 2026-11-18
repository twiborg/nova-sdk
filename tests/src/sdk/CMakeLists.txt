add_executable(nova_sdk_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/application_test.cpp
)
target_link_libraries(nova_sdk_tests PRIVATE nova_sdk GTest::gtest_main)
add_test(NAME nova_sdk_tests COMMAND nova_sdk_tests)
