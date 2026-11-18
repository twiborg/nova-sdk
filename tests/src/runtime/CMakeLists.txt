add_executable(nova_runtime_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_context_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/task/task_scheduler_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event/event_dispatcher_test.cpp
)
target_link_libraries(nova_runtime_tests PRIVATE nova_runtime GTest::gtest_main)
add_test(NAME nova_runtime_tests COMMAND nova_runtime_tests)
