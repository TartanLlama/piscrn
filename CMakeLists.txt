cmake_minimum_required(VERSION 3.14)

project(piscrn VERSION 0.1.0 LANGUAGES C)

find_package(libpng REQUIRED)
find_package(ZLIB REQUIRED)

add_library(libpiscrn src/libpiscrn.c)
target_link_libraries(libpiscrn PUBLIC png_static ZLIB::ZLIB bcm_host vcos vchiq_arm)
target_include_directories(libpiscrn
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(libpiscrn PROPERTIES OUTPUT_NAME piscrn)

add_executable(piscrn src/piscrn.c)
target_link_libraries(piscrn PRIVATE libpiscrn)

add_executable(piscrnd src/piscrnd.c)
target_link_libraries(piscrnd PRIVATE libpiscrn)

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/piscrn)

install(TARGETS libpiscrn piscrn piscrnd
    EXPORT piscrn-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#Export the targets to a script
install(EXPORT piscrn-targets
    FILE
        piscrn-targets.cmake
    NAMESPACE
        piscrn::
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/piscrn-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/piscrn-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/piscrn-config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/piscrn-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/piscrn-config-version.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

##############################################
## Exporting from the build tree
export(EXPORT piscrn-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/piscrn-targets.cmake
    NAMESPACE piscrn::)

#Register package in user's package registry
export(PACKAGE piscrn)

