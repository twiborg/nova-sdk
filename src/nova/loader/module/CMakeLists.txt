add_library(nova_loader_module
    ${CMAKE_CURRENT_SOURCE_DIR}/module_enumerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/module_error.cpp
)

target_include_directories(nova_loader_module
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nova_loader_module
    PUBLIC
        nova_core
        psapi.lib
)

target_compile_features(nova_loader_module PUBLIC cxx_std_20)
