add_library(nova_runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/task/task_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event/event_dispatcher.cpp
)

target_include_directories(nova_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nova_runtime
    PUBLIC
        nova_loader
        nova_core
)

target_compile_features(nova_runtime PUBLIC cxx_std_20)
