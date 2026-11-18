add_executable(nova_loader_module_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/module_enumerator_test.cpp
)
target_link_libraries(nova_loader_module_tests PRIVATE nova_loader_module GTest::gtest_main)
add_test(NAME nova_loader_module_tests COMMAND nova_loader_module_tests)
