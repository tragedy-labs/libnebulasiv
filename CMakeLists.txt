cmake_minimum_required(VERSION 3.13)
project(libnebulasiv VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Emit build/compile_commands.json so editors/clangd know the include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

# Capability modules + transport. All command logic lives here; per-chip
# bundle headers (include/um98x.h) only aggregate these.
add_library(nebulasiv STATIC
  src/serial.c
  src/neb_serial_transport.c
  src/neb_core.c
  src/neb_mode.c
  src/neb_config.c
  src/neb_rtk.c
  src/neb_mask.c
  src/neb_assist.c
  src/neb_heading.c
  src/neb_admin.c
  src/neb_logging.c
)

# Consumers link nebulasiv::nebulasiv whether they found an installed copy or
# added this directory to their own build, so the two paths are interchangeable.
add_library(nebulasiv::nebulasiv ALIAS nebulasiv)

set_target_properties(nebulasiv PROPERTIES
  VERSION ${PROJECT_VERSION}
  OUTPUT_NAME nebulasiv
)

include(GNUInstallDirs)

# Headers come from the source tree when building here, and from the install
# prefix once installed -- the same target works both ways.
target_include_directories(nebulasiv PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
# Off by default when this project is pulled into a larger build, so a consumer
# doing add_subdirectory() does not inherit our install rules.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(NEBULASIV_INSTALL_DEFAULT ON)
else()
  set(NEBULASIV_INSTALL_DEFAULT OFF)
endif()
option(NEBULASIV_INSTALL "Generate install rules for libnebulasiv"
       ${NEBULASIV_INSTALL_DEFAULT})

if(NEBULASIV_INSTALL)
  include(CMakePackageConfigHelpers)

  install(TARGETS nebulasiv
    EXPORT nebulasivTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
  )

  install(EXPORT nebulasivTargets
    FILE nebulasivTargets.cmake
    NAMESPACE nebulasiv::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nebulasiv
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nebulasivConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/nebulasivConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nebulasiv
  )

  # SameMajorVersion: while the major version is 0 every minor bump is treated
  # as breaking, which is what an API still being shaped needs.
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/nebulasivConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/nebulasivConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/nebulasivConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nebulasiv
  )

  # For consumers not using CMake. The .pc locates the prefix relative to its
  # own directory, so compute how far that is -- this depends only on LIBDIR,
  # not on the prefix, and so stays correct if the tree is installed elsewhere.
  file(RELATIVE_PATH NEBULASIV_PC_RELATIVE_PREFIX
    "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
  string(REGEX REPLACE "/$" "" NEBULASIV_PC_RELATIVE_PREFIX
    "${NEBULASIV_PC_RELATIVE_PREFIX}")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nebulasiv.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/nebulasiv.pc
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nebulasiv.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()
