cmake_minimum_required(VERSION 3.27.0)

if (TARGET Basic++)
    return()
endif()

set(BASIC_VERSION 2.1.2) 

set(LIBRARY_OUTPUT_NAME "Basic++.${BASIC_VERSION}")

project(Basic++
    VERSION ${BASIC_VERSION}
    LANGUAGES CXX
    HOMEPAGE_URL https://github.com/BasisDeveloper/BasicPlusPlus)

# -- Setting the output directory of the package to have the version number appended:
# ( there is probably a better way to do this utilizing generator expressions. 
#   As I could know the install directory at install time. 
#   But this simple solution works for now.)
set(PACKAGE_INSTALL_DIRECTORY "")
cmake_path(GET CMAKE_INSTALL_PREFIX PARENT_PATH PACKAGE_INSTALL_DIRECTORY)
if (NOT PACKAGE_INSTALL_DIRECTORY)
    message(FATAL_ERROR "the install prefix couldn't be calculated.")
endif()
set(CMAKE_INSTALL_PREFIX ${PACKAGE_INSTALL_DIRECTORY}/${LIBRARY_OUTPUT_NAME})


set(INCLUDE_DIR include/Basic++)

set(headers
    ${INCLUDE_DIR}/Common.hxx
    ${INCLUDE_DIR}/dbg.hxx
    ${INCLUDE_DIR}/Defer.hxx
    ${INCLUDE_DIR}/NumericAliases.hxx
    ${INCLUDE_DIR}/Expectations.hxx
    ${INCLUDE_DIR}/Formatting.hxx
    ${INCLUDE_DIR}/Printing.hxx
    ${INCLUDE_DIR}/Mathematics.hxx
    ${INCLUDE_DIR}/Message.hxx
    ${INCLUDE_DIR}/Sys.hpp
    ${INCLUDE_DIR}/TypeName.hxx
    ${INCLUDE_DIR}/win32/Win32.hpp
    ${INCLUDE_DIR}/IO.hxx
    ${INCLUDE_DIR}/Expected.hxx
)

set(sources
    src/Sys.cpp
    src/Win32.cpp
)

add_library(Basic++ STATIC ${headers} ${sources})
add_library(Basic++::Basic++ ALIAS Basic++)

target_compile_features(Basic++ PUBLIC cxx_std_23)

target_link_libraries(Basic++ PUBLIC dbghelp)

#set(CMAKE_INSTALL_PREFIX Basic++)


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

#install(DIRECTORY ${CMAKE_SOURCE_DIR} DESTINATION your_target_directory)

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
    DIRECTORY ${INCLUDE_DIR}
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
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_OUTPUT_NAME}")

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Basic++Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Basic++
)

set(PACKAGE_VERSION ${BASIC++_MAJOR_VERSION})

set_target_properties(Basic++ PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${PACKAGE_VERSION}
    COMPATIBLE_INTERFACE_STRING Basic++_MAJOR_VERSION)

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++ConfigVersion.cmake"
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Basic++Config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_OUTPUT_NAME}"
)

# https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html#exporting-targets-from-the-build-tree
export(EXPORT Basic++Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/Basic++Targets.cmake"
    NAMESPACE Basic++::
)

add_subdirectory(Sandbox/)