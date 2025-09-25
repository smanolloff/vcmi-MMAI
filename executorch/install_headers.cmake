# This file is used when compiling executorch from source and
# must be copied into the executorch root dir as cmake_install.cmake.
# See MMAI/CMakeLists.txt for more information.

# Executorch's high-level API headers (such as extension/module.h) are
# *not* installed by default for some reason. This script installs them.

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX is undefined.")
endif()

foreach(dir IN ITEMS extension runtime)
  file(INSTALL
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include/executorch"
    FILES_MATCHING
    PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.hh" PATTERN "*.hxx")
endforeach()
