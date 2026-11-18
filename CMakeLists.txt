cmake_minimum_required(VERSION 3.24)

project(NOVA
    VERSION 0.2.0
    LANGUAGES CXX
)

set(NOVA_CXX_STANDARD 20 CACHE STRING "C++ language standard")
set(NOVA_BUILD_TESTS ON CACHE BOOL "Build unit tests")
set(NOVA_BUILD_EXAMPLES ON CACHE BOOL "Build example executable")
set(NOVA_BUILD_EXAMPLES_EXTERNAL ON CACHE BOOL "Build external integration examples")

if (MSVC)
    add_compile_options(/W4 /WX /permissive- /Zc:__cplusplus)
endif()

set(CMAKE_CXX_STANDARD ${NOVA_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Expose version as compile definitions
add_compile_definitions(
    NOVA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NOVA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NOVA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    NOVA_VERSION_STRING="${PROJECT_VERSION}"
)

include(CTest)

option(NOVA_FETCH_GTEST "Fetch GoogleTest via FetchContent" ON)

add_subdirectory(src/nova/core)
add_subdirectory(src/nova/loader)
add_subdirectory(src/nova/runtime)
add_subdirectory(src/nova/sdk)
add_subdirectory(tests)

# Public alias targets for consumers
add_library(nova::core ALIAS nova_core)
add_library(nova::loader ALIAS nova_loader)
add_library(nova::runtime ALIAS nova_runtime)
add_library(nova::sdk ALIAS nova_sdk)

if (NOVA_BUILD_EXAMPLES)
    add_executable(nova_core_example
        src/nova/core/main.cpp
    )
    target_link_libraries(nova_core_example PRIVATE nova_core)
    target_include_directories(nova_core_example PRIVATE include)
endif()

if (NOVA_BUILD_EXAMPLES_EXTERNAL)
    add_subdirectory(examples/external_application)
endif()

if (NOVA_BUILD_TESTS)
    add_subdirectory(tests/integration/external_consumer)
endif()

# ---------------------------------------------------------------------------
# Installation support
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

set(NOVA_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}" CACHE STRING "Installation directory for libraries")
set(NOVA_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE STRING "Installation directory for headers")
set(NOVA_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/nova" CACHE STRING "Installation directory for CMake config files")

# Install public headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${NOVA_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install targets
install(TARGETS nova_core nova_loader_pe nova_loader_module nova_loader nova_runtime nova_sdk
    EXPORT novaTargets
    LIBRARY DESTINATION ${NOVA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${NOVA_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Export targets for find_package()
include(CMakePackageConfigHelpers)
install(TARGETS nova_core nova_loader_pe nova_loader_module nova_loader nova_runtime nova_sdk
    LIBRARY DESTINATION ${NOVA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${NOVA_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT novaTargets
    NAMESPACE nova::
    FILE novaTargets.cmake
    DESTINATION ${NOVA_INSTALL_CMAKEDIR}
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/novaConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/novaConfig.cmake
    INSTALL_DESTINATION ${NOVA_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/novaConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/novaConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/novaConfigVersion.cmake
    DESTINATION ${NOVA_INSTALL_CMAKEDIR}
)
