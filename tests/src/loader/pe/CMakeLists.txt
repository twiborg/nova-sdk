add_executable(nova_loader_pe_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/pe_parser_test.cpp
)
target_link_libraries(nova_loader_pe_tests PRIVATE nova_loader_pe GTest::gtest_main)
add_test(NAME nova_loader_pe_tests COMMAND nova_loader_pe_tests)
