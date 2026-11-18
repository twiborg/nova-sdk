cmake_minimum_required(VERSION 3.24)

project(ExternalExample
    VERSION 1.0.0
    LANGUAGES CXX
)

add_executable(external_example
    src/main.cpp
)

target_link_libraries(external_example PRIVATE nova::sdk)
target_include_directories(external_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
