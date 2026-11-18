add_executable(nova_loader_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/loader_test.cpp
)
target_link_libraries(nova_loader_tests PRIVATE nova_loader GTest::gtest_main)
add_test(NAME nova_loader_tests COMMAND nova_loader_tests)
