add_library(nova_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/application.cpp
)

target_include_directories(nova_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nova_sdk
    PUBLIC
        nova_runtime
        nova_loader
        nova_core
)

target_compile_features(nova_sdk PUBLIC cxx_std_20)
