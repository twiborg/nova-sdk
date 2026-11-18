cmake_minimum_required(VERSION 3.24)

project(ExternalConsumer
    VERSION 1.0.0
    LANGUAGES CXX
)

add_executable(external_consumer
    src/main.cpp
)

target_link_libraries(external_consumer PRIVATE nova::sdk)
target_include_directories(external_consumer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
