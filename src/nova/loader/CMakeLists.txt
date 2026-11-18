add_subdirectory(pe)
add_subdirectory(module)

add_library(nova_loader
    ${CMAKE_CURRENT_SOURCE_DIR}/loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader_result.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader_version.cpp
)

target_include_directories(nova_loader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nova_loader
    PUBLIC
        nova_loader_pe
        nova_loader_module
        nova_core
)

target_compile_features(nova_loader PUBLIC cxx_std_20)
