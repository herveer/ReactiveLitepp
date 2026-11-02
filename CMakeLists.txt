cmake_minimum_required(VERSION 3.14)

project(ReactiveLitepp
    VERSION 1.2.1
    DESCRIPTION "Type-safe reactive programming inspired by INotifyPropertyChanged"
    HOMEPAGE_URL "https://github.com/herve-er/ReactiveLitepp"
    LANGUAGES CXX
)

# --------------------------------------------
# Dependencies
# --------------------------------------------
find_package(nameof CONFIG REQUIRED)

# --------------------------------------------
# Header-only library
# --------------------------------------------
add_library(ReactiveLitepp INTERFACE)
add_library(ReactiveLitepp::ReactiveLitepp ALIAS ReactiveLitepp)

target_include_directories(ReactiveLitepp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ReactiveLitepp INTERFACE cxx_std_20)

target_link_libraries(ReactiveLitepp INTERFACE nameof::nameof)

# --------------------------------------------
# Install rules
# --------------------------------------------
include(GNUInstallDirs)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(TARGETS ReactiveLitepp
    EXPORT ReactiveLiteppTargets
)

# --------------------------------------------
# CMake package config files
# --------------------------------------------
include(CMakePackageConfigHelpers)

# The package's files carry ONE spelling, the all-lowercase one find_package(ReactiveLitepp)
# looks for on a case-sensitive file system (<lowercase name>-config.cmake), whatever the
# project is called - so it installs the same on Windows, Linux and macOS, with or without vcpkg.
set(PACKAGE_FILE_NAME reactivelitepp)
set(CONFIG_INSTALL_DIR share/${PACKAGE_FILE_NAME})

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_FILE_NAME}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_FILE_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

# Install config + version files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_FILE_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_FILE_NAME}-config-version.cmake"
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Install exported targets file
install(EXPORT ReactiveLiteppTargets
    FILE ${PACKAGE_FILE_NAME}-targets.cmake
    NAMESPACE ReactiveLitepp::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# --------------------------------------------
# Options
# --------------------------------------------
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()

if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()