cmake_minimum_required(VERSION 3.27.0)

if (TARGET Basic++)
    return()
endif()

project(Basic++
    VERSION 3.0.0
    LANGUAGES CXX
    HOMEPAGE_URL https://github.com/BasisDeveloper/BasicPlusPlus)

set(include_dir include/Basic++)

set(headers
    ${include_dir}/Common.hxx
    ${include_dir}/dbg.hxx
    ${include_dir}/Defer.hxx
    ${include_dir}/NumericAliases.hxx
    ${include_dir}/Expectations.hxx
    ${include_dir}/Formatting.hxx
    ${include_dir}/Printing.hxx
    ${include_dir}/Mathematics.hxx
    ${include_dir}/Message.hxx
    ${include_dir}/Sys.hpp
    ${include_dir}/TypeName.hxx
    ${include_dir}/win32/Win32.hpp
    ${include_dir}/IO.hxx
    ${include_dir}/Result.hxx
)

set(sources
    src/Sys.cpp
    src/Win32.cpp
)

add_library(Basic++ STATIC ${headers} ${sources})
add_library(Basic++::Basic++ ALIAS Basic++)

target_compile_features(Basic++ PUBLIC cxx_std_23)

target_link_libraries(Basic++ PUBLIC dbghelp)

target_include_directories(Basic++
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/win32>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
    src/
)

# :: INSTALL :: #
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS Basic++
    EXPORT Basic++Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Basic++_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Basic++_Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Basic++_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
    DIRECTORY ${include_dir}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
    FILES_MATCHING
    PATTERN "*.hxx"
    PATTERN "*.txx"
    PATTERN "*.hpp"
    PATTERN "*.h")

install(
    EXPORT Basic++Targets
    FILE Basic++Targets.cmake
    NAMESPACE Basic++::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Basic++")

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Basic++Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Basic++
)

set(package_version ${BASIC++_MAJOR_VERSION})

set_target_properties(Basic++ PROPERTIES
    VERSION ${package_version}
    SOVERSION ${package_version}
    COMPATIBLE_INTERFACE_STRING Basic++_MAJOR_VERSION)

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++ConfigVersion.cmake"
    VERSION ${package_version}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++Config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Basic++"
)

# https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html#exporting-targets-from-the-build-tree
export(EXPORT Basic++Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/Basic++Targets.cmake"
    NAMESPACE Basic++::
)

add_subdirectory(Sandbox/)