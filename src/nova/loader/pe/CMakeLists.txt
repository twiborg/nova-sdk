add_library(nova_loader_pe
    ${CMAKE_CURRENT_SOURCE_DIR}/pe_parser.cpp
)

target_include_directories(nova_loader_pe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nova_loader_pe
    PUBLIC
        nova_core
)

target_compile_features(nova_loader_pe PUBLIC cxx_std_20)
