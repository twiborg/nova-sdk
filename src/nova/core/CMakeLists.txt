add_library(nova_core
    ${CMAKE_CURRENT_SOURCE_DIR}/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
)

target_include_directories(nova_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(nova_core PUBLIC cxx_std_20)

if (NOVA_BUILD_TESTS AND TARGET gtest)
    target_link_libraries(nova_core PRIVATE gtest_main)
endif()
