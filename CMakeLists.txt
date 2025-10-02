cmake_minimum_required(VERSION 3.24)

set(MMAI_FILES
  BAI/base.cpp
  BAI/base.h
  BAI/router.cpp
  BAI/router.h
  BAI/model/ScriptedModel.h
  BAI/model/ScriptedModel.cpp
  # BAI/model/TorchModel.h
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
set(MMAI_LIBTORCH_PATH "" CACHE PATH "Link against libtorch located at the specified path (don't link if empty)")

set(EXECUTORCH_LIBRARIES "")

set(MMAI_LIBS vcmi)
set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
set(MMAI_THIRD_PARTY_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/schema/gcem)

#[[
About ExecuTorch vs. Libtorch:
Executorch is a more "modern" and flexible alternative to Libtorch.
Executorch must not be used on Windows, as it causes memory corruption.
Executorch's XNNPACK-lowered models run ~3x slower than libtorch's optimized
models on Mac M1.
Executorch is linked statically to VCMI and has a tiny size footprint.
Libtorch is linked dynamically (except for mobile) and is huge in size (100MB).
Executorch build is super fast (few minutes at most), compared to libtorch
which requires *several houts* to build. This is why libtorch is pre-built
on CI, but such builds regularly need to re-run to keep up with upstream
updates (e.g. new pytorch releases). If the builds stop working, it will be
difficult to troubleshoot the issue given the huge build times.
---
Ultimately, runtime performance matters most, so libtorch should be preferred.
The only reason keep the door open for executorch would be:
- future potential (when the project matures enough)
- edge device support, e.g. it compiles on android32 (libtorch does not).
  However, I have NOT tested it on such a device.
]]

if(MMAI_EXECUTORCH_INSTALL_DIR)
  if(MMAI_LIBTORCH_PATH)
    message(FATAL_ERROR "MMAI_EXECUTORCH_INSTALL_DIR and MMAI_LIBTORCH_PATH are mutually exclusive.")
  endif()

  message(WARNING "ExecuTorch does causes memory corruption on windows. Prefer libtorch.")

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
    $ . .venv/bin/activate

    # Copy cmake files from MMAI dir
    $ {vcmi_dir}/executorch/install_headers.cmake ./
    $ {vcmi_dir}/executorch/CMakeUserPresets.json ./

    $ cmake --preset mmai-executorch-release
    $ cmake --build --preset mmai-executorch-release -j10
    $ cmake --preset mmai-executorch-release -P install_headers.cmake

  Now you can build VCMI with:
    MMAI_EXECUTORCH_INSTALL_DIR=/some/path/excutorch/install-release
  ]]

  add_definitions(-DUSING_EXECUTORCH=1)

  if (ENABLE_MMAI_STRICT_LOAD)
    add_definitions(-DENABLE_MMAI_STRICT_LOAD=1)
  endif()

  set(executorch_ROOT "${MMAI_EXECUTORCH_INSTALL_DIR}")
  find_package(executorch CONFIG REQUIRED)

  message(STATUS "EXECUTORCH_FOUND: ${EXECUTORCH_FOUND}")
  message(STATUS "EXECUTORCH_INCLUDE_DIRS: ${EXECUTORCH_INCLUDE_DIRS}")
  message(STATUS "EXECUTORCH_LIBRARIES: ${EXECUTORCH_LIBRARIES}")

  list(APPEND MMAI_THIRD_PARTY_INCLUDES ${EXECUTORCH_INCLUDE_DIRS})
  list(APPEND MMAI_FILES BAI/model/TorchModel_ET.h)
  list(APPEND MMAI_FILES BAI/model/TorchModel_ET.cpp)

  function(force_link_static TARGET LIBDIR)
    foreach(lib IN LISTS ARGN)
      if(NOT lib IN_LIST EXECUTORCH_LIBRARIES)
        message(STATUS "force_link_static: skip ${lib} (not in EXECUTORCH_LIBRARIES)")
        continue()
      endif()

      message(STATUS "force_link_static: ${lib}")

      # MSVC uses foo.lib, others use libfoo.a
      set(_path "${LIBDIR}/$<IF:$<BOOL:${MSVC}>,,lib>${lib}$<IF:$<BOOL:${MSVC}>,.lib,.a>")
      target_link_libraries(${TARGET} PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,${_path}>)
    endforeach()
  endfunction()

  set(_force_libs
    xnnpack_backend
    optimized_kernels
    quantized_kernels
    portable_ops_lib
  )

  set(_nonforce_libs ${EXECUTORCH_LIBRARIES})
  list(REMOVE_ITEM _nonforce_libs ${_force_libs})

  message(STATUS "_force_libs: ${_force_libs}")
  message(STATUS "_nonforce_libs: ${_nonforce_libs}")
elseif(MMAI_LIBTORCH_PATH)
  #[[
  About the MMAI_LIBTORCH_PATH flag:

  MMAI_LIBTORCH_PATH must point to a lightweight (CPU-only) version of "libtorch"
  which will be used by VCMI for loading pre-trained ML models during gameplay.
  Depending on what VCMI is being compiled for:
  a) gameplay (i.e. if ENABLE_ML=0): MMAI_LIBTORCH_PATH must be a valid path
  b) training (i.e. if ENABLE_ML=1): MMAI_LIBTORCH_PATH must be blank

  Libtorch should only be used on windows, where executorch is not an option.
  ]]

  add_definitions(-DUSING_LIBTORCH=1)
  list(APPEND CMAKE_PREFIX_PATH "${MMAI_LIBTORCH_PATH}")
  find_package(Torch REQUIRED)
  if(APPLE_IOS)
    # Found in system SDK, somewhere in `xcrun --sdk iphoneos --show-sdk-path`
    find_library(ACCELERATE Accelerate)
    list(APPEND MMAI_LIBS "${ACCELERATE}")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

  list(APPEND MMAI_LIBS "${TORCH_LIBRARIES}")
  list(APPEND MMAI_THIRD_PARTY_INCLUDES "${TORCH_INCLUDE_DIRS}")
  list(APPEND MMAI_FILES BAI/model/TorchModel_LT.h)
  list(APPEND MMAI_FILES BAI/model/TorchModel_LT.cpp)

  message("MMAI_LIBS: ${MMAI_LIBS}")
  message("MMAI_FILES: ${MMAI_INCLUDES}")
  # message(STATUS "TORCH_INCLUDE_DIRS: ${TORCH_INCLUDE_DIRS}")
  # message(STATUS "TORCH_CXX_FLAGS: ${TORCH_CXX_FLAGS}")
  # message(STATUS "TORCH_LIBRARIES: ${TORCH_LIBRARIES}")

  function(install_torch_libs)
    foreach(libname IN LISTS ARGN)
      unset(${libname}_LIBRARY CACHE)
      find_library(${libname}_LIBRARY ${libname} PATHS "${TORCH_INSTALL_PREFIX}/lib")
      set(libpath ${${libname}_LIBRARY})

      if(MSVC)
        string(REPLACE ".lib" ".dll" libpath ${libpath})
        add_custom_command(TARGET MMAI POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libpath}" "$<TARGET_FILE_DIR:vcmi>"
        )
      endif()

      message("Found ${libname}: ${libpath}")
      install(FILES ${libpath} DESTINATION ${LIB_DIR})
    endforeach()
  endfunction(install_torch_libs)
else()
  if(NOT ENABLE_ML)
    message(FATAL_ERROR "Either MMAI_EXECUTORCH_INSTALL_DIR or MMAI_LIBTORCH_PATH is required unless ENABLE_ML is set")
  endif()

  list(APPEND MMAI_FILES BAI/model/TorchModel_dummy.h)
  list(APPEND MMAI_FILES BAI/model/TorchModel_dummy.cpp)
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

if (MMAI_LIBTORCH_PATH AND NOT (APPLE_IOS OR ANDROID))
  # libtorch libs are static on mobile
  install_torch_libs(torch torch_cpu c10)
endif()
if (MMAI_EXECUTORCH_INSTALL_DIR)
  force_link_static(MMAI "${MMAI_EXECUTORCH_INSTALL_DIR}/lib" ${_force_libs})
endif()

message(STATUS "MMAI target_link_libraries: ${MMAI_LIBS}")
target_link_libraries(MMAI PRIVATE ${MMAI_LIBS})
message(STATUS "MMAI target_link_libraries: ${_nonforce_libs}")
target_link_libraries(MMAI PRIVATE ${_nonforce_libs})

target_link_options(MMAI PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/VERBOSE:LIB>)

# Used in schema/base.h to determine if it is imported by MMAI or vcmi-gym
set_target_properties(MMAI PROPERTIES COMPILE_DEFINITIONS "MMAI_DLL=1")

message(STATUS "MMAI target_include_directories: ${MMAI_INCLUDES}")
target_include_directories(MMAI PUBLIC ${MMAI_INCLUDES})

# Mark as SYSTEM to prevent -Werror failing builds due to warnings from these
message(STATUS "MMAI target_include_directories: ${MMAI_THIRD_PARTY_INCLUDES}")
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
