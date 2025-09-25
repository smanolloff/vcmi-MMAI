set(MMAI_FILES
  BAI/base.cpp
  BAI/base.h
  BAI/router.cpp
  BAI/router.h
  BAI/model/ScriptedModel.h
  BAI/model/ScriptedModel.cpp
  BAI/model/TorchModel.h
  # BAI/model/TorchModel.cpp       # optionally added later
  # BAI/model/TorchModelDummy.cpp  # optionally added later

  BAI/v12/BAI.cpp
  BAI/v12/BAI.h
  BAI/v12/action.cpp
  BAI/v12/action.h
  BAI/v12/attack_log.h
  BAI/v12/battlefield.cpp
  BAI/v12/battlefield.h
  BAI/v12/encoder.cpp
  BAI/v12/encoder.h
  BAI/v12/global_stats.cpp
  BAI/v12/global_stats.h
  BAI/v12/hex.cpp
  BAI/v12/hex.h
  BAI/v12/hexaction.h
  BAI/v12/hexactmask.h
  BAI/v12/player_stats.cpp
  BAI/v12/player_stats.h
  BAI/v12/render.cpp
  BAI/v12/render.h
  BAI/v12/stack.cpp
  BAI/v12/stack.h
  BAI/v12/state.cpp
  BAI/v12/state.h
  BAI/v12/supplementary_data.cpp
  BAI/v12/supplementary_data.h
  BAI/v12/util.cpp
  BAI/v12/util.h

  BAI/v13/BAI.cpp
  BAI/v13/BAI.h
  BAI/v13/action.cpp
  BAI/v13/action.h
  BAI/v13/attack_log.h
  BAI/v13/battlefield.cpp
  BAI/v13/battlefield.h
  BAI/v13/encoder.cpp
  BAI/v13/encoder.h
  BAI/v13/global_stats.cpp
  BAI/v13/global_stats.h
  BAI/v13/hex.cpp
  BAI/v13/hex.h
  BAI/v13/hexaction.h
  BAI/v13/hexactmask.h
  BAI/v13/links.h
  BAI/v13/player_stats.cpp
  BAI/v13/player_stats.h
  BAI/v13/render.cpp
  BAI/v13/render.h
  BAI/v13/stack.cpp
  BAI/v13/stack.h
  BAI/v13/state.cpp
  BAI/v13/state.h
  BAI/v13/supplementary_data.cpp
  BAI/v13/supplementary_data.h
  BAI/v13/util.cpp
  BAI/v13/util.h

  schema/schema.h
  schema/v12/constants.h
  schema/v12/expbin.h
  schema/v12/linbin.h
  schema/v12/schema.h
  schema/v12/types.h
  schema/v12/util.h
  schema/v13/constants.h
  schema/v13/expbin.h
  schema/v13/linbin.h
  schema/v13/schema.h
  schema/v13/types.h
  schema/v13/util.h


  MMAI.h
  StdInc.h
  common.h
)

option(ENABLE_MMAI_TEST "Compile tests" OFF)
option(ENABLE_MMAI_STRICT_LOAD "Disable MMAI fallback during model load and throw an error instead" OFF)
set(MMAI_EXECUTORCH_INSTALL_DIR "" CACHE PATH "Path to executorch v0.7.0 install directory")

#[[
About the MMAI_EXECUTORCH_INSTALL_DIR flag:

`executorch` is the library which handles loading pre-trained ML models during
gameplay. It must be compiled externally so VCMI can be linked against it.
MMAI_EXECUTORCH_INSTALL_DIR contains header files and build artifacts (static libs).

Example setup:

  $ cd /some/path
  $ git clone --recurse-submodules -b release/0.7 https://github.com/pytorch/executorch.git
  $ cd executorch
  $ python3 -m venv .venv
  $ . ./venv/bin/activate

  # Copy cmake files from MMAI dir
  $ {vcmi_dir}/executorch/install_headers.cmake ./
  $ {vcmi_dir}/executorch/CMakeUserPresets.json ./

  $ cmake --preset mmai-executorch-release
  $ cmake --build --preset mmai-executorch-release -j10
  $ cmake --preset mmai-executorch-release -P install_headers.cmake

Now you can build VCMI with:
  MMAI_EXECUTORCH_INSTALL_DIR=/some/path/excutorch/install-release
]]

set(MMAI_LIBS vcmi)
set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
set(MMAI_THIRD_PARTY_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/schema/gcem)

if(MMAI_EXECUTORCH_INSTALL_DIR)
  add_definitions(-DUSING_EXECUTORCH=1)

  if (ENABLE_MMAI_STRICT_LOAD)
    add_definitions(-DENABLE_MMAI_STRICT_LOAD=1)
  endif()

  set(executorch_ROOT "${MMAI_EXECUTORCH_INSTALL_DIR}")
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
  find_package(executorch CONFIG REQUIRED)

  message(STATUS "EXECUTORCH_FOUND: ${EXECUTORCH_FOUND}")
  message(STATUS "EXECUTORCH_INCLUDE_DIRS: ${EXECUTORCH_INCLUDE_DIRS}")
  message(STATUS "EXECUTORCH_LIBRARIES: ${EXECUTORCH_LIBRARIES}")

  list(APPEND MMAI_THIRD_PARTY_INCLUDES ${EXECUTORCH_INCLUDE_DIRS})
  list(APPEND MMAI_LIBS ${EXECUTORCH_LIBRARIES})
  list(APPEND MMAI_FILES BAI/model/TorchModel.cpp)
else()
  if(NOT ENABLE_ML)
    message(FATAL_ERROR "MMAI_EXECUTORCH_INSTALL_DIR is required unless ENABLE_ML is set")
  endif()

  list(APPEND MMAI_FILES BAI/model/TorchModelDummy.cpp)
endif()

if(NOT ENABLE_STATIC_LIBS)
  list(APPEND MMAI_FILES main.cpp StdInc.cpp)
endif()

assign_source_group(${MMAI_FILES})

if(ENABLE_STATIC_LIBS)
  add_library(MMAI STATIC ${MMAI_FILES})
else()
  add_library(MMAI SHARED ${MMAI_FILES})
  install(TARGETS MMAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()


# Inputs:
#   TARGET  - your target (e.g., MMAI)
#   LIBDIR  - absolute path to the directory with the archives
#   LIBS    - list of bare library base names (no prefix, no extension)
function(force_link_static TARGET LIBDIR)
  set(_libs ${ARGN})

  # Prefix/ext differ across toolchains
  set(_prefix $<$<BOOL:${MSVC}>:>$<$<NOT:$<BOOL:${MSVC}>>:lib>)
  set(_ext    $<$<BOOL:${MSVC}>:.lib>$<$<NOT:$<BOOL:${MSVC}>>:.a>)

  foreach(lib IN LISTS _libs)
    set(_path "${LIBDIR}/${_prefix}${lib}${_ext}")

    if(APPLE)
      target_link_options(${TARGET} PRIVATE "-Wl,-force_load,${_path}")
    elseif(MSVC)
      target_link_options(${TARGET} PRIVATE "/WHOLEARCHIVE:${_path}")
    else()
      target_link_options(${TARGET} PRIVATE "-Wl,--whole-archive,${_path}" "-Wl,--no-whole-archive")
    endif()

    # Keep the archive on the link line too
    target_link_libraries(${TARGET} PRIVATE "${_path}")
  endforeach()
endfunction()

force_link_static(MMAI "${MMAI_EXECUTORCH_INSTALL_DIR}/lib"
  extension_module_static
  extension_data_loader
  extension_tensor
  extension_flat_tensor
  xnnpack_backend
  optimized_kernels
  quantized_kernels
  portable_ops_lib)

target_link_libraries(MMAI PUBLIC ${MMAI_LIBS})
set_target_properties(MMAI PROPERTIES COMPILE_DEFINITIONS "MMAI_DLL=1")
target_include_directories(MMAI PUBLIC ${MMAI_INCLUDES})

# Mark as SYSTEM to prevent -Werror failing builds due to warnings from these
target_include_directories(MMAI SYSTEM PRIVATE ${MMAI_THIRD_PARTY_INCLUDES})

if(ENABLE_MMAI_TEST)
  include(GoogleTest)
  include(CheckCXXCompilerFlag)
  enable_testing()

  target_link_libraries(MMAI PUBLIC gtest gtest_main)

  target_include_directories(MMAI PRIVATE "${CMAKE_SOURCE_DIR}/test/googletest/googletest/include")
  add_subdirectory(${CMAKE_SOURCE_DIR}/test/googletest ${CMAKE_SOURCE_DIR}/test/googletest/build EXCLUDE_FROM_ALL)
  add_executable(MMAI_test test/encoder_test.cpp)
  target_link_libraries(MMAI_test PRIVATE MMAI)
  gtest_discover_tests(MMAI_test)

  # default visibility is needed for testing
  set_target_properties(MMAI PROPERTIES CXX_VISIBILITY_PRESET "default")
  set_target_properties(MMAI_test PROPERTIES CXX_VISIBILITY_PRESET "default")

  # Run tests with:
  # ctest --test-dir build/AI/MMAI/
endif()

vcmi_set_output_dir(MMAI "AI")
enable_pch(MMAI)
