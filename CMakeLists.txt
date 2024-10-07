set(MMAI_FILES
  AAI/AAI.cpp
  AAI/AAI.h

  BAI/base.cpp
  BAI/base.h
  BAI/router.cpp
  BAI/router.h
  BAI/scripted/summoner.cpp
  BAI/scripted/summoner.h
  BAI/model/ScriptedModel.h
  BAI/model/ScriptedModel.cpp
  BAI/model/TorchModel.h
  # BAI/model/TorchModel.cpp       # optionally added later
  # BAI/model/TorchModelDummy.cpp  # optionally added later

  BAI/v1/BAI.cpp
  BAI/v1/BAI.h
  BAI/v1/action.cpp
  BAI/v1/action.h
  BAI/v1/attack_log.h
  BAI/v1/battlefield.cpp
  BAI/v1/battlefield.h
  BAI/v1/encoder.cpp
  BAI/v1/encoder.h
  BAI/v1/hex.cpp
  BAI/v1/hex.h
  BAI/v1/hexaction.h
  BAI/v1/hexactmask.h
  BAI/v1/render.cpp
  BAI/v1/render.h
  BAI/v1/stackinfo.h
  BAI/v1/state.cpp
  BAI/v1/state.h
  BAI/v1/supplementary_data.cpp
  BAI/v1/supplementary_data.h

  BAI/v2/BAI.cpp
  BAI/v2/BAI.h
  BAI/v2/encoder.cpp
  BAI/v2/encoder.h
  BAI/v2/state.cpp
  BAI/v2/state.h

  BAI/v3/BAI.cpp
  BAI/v3/BAI.h
  BAI/v3/action.cpp
  BAI/v3/action.h
  BAI/v3/attack_log.h
  BAI/v3/battlefield.cpp
  BAI/v3/battlefield.h
  BAI/v3/encoder.cpp
  BAI/v3/encoder.h
  BAI/v3/general_info.cpp
  BAI/v3/general_info.h
  BAI/v3/hex.cpp
  BAI/v3/hex.h
  BAI/v3/hexaction.h
  BAI/v3/hexactmask.h
  BAI/v3/render.cpp
  BAI/v3/render.h
  BAI/v3/stack.cpp
  BAI/v3/stack.h
  BAI/v3/state.cpp
  BAI/v3/state.h
  BAI/v3/supplementary_data.cpp
  BAI/v3/supplementary_data.h

  BAI/v4/BAI.cpp
  BAI/v4/BAI.h
  BAI/v4/action.cpp
  BAI/v4/action.h
  BAI/v4/attack_log.h
  BAI/v4/battlefield.cpp
  BAI/v4/battlefield.h
  BAI/v4/encoder.cpp
  BAI/v4/encoder.h
  BAI/v4/general_info.cpp
  BAI/v4/general_info.h
  BAI/v4/hex.cpp
  BAI/v4/hex.h
  BAI/v4/hexaction.h
  BAI/v4/hexactmask.h
  BAI/v4/render.cpp
  BAI/v4/render.h
  BAI/v4/stack.cpp
  BAI/v4/stack.h
  BAI/v4/state.cpp
  BAI/v4/state.h
  BAI/v4/supplementary_data.cpp
  BAI/v4/supplementary_data.h
  BAI/v4/util.cpp
  BAI/v4/util.h

  schema/schema.h
  schema/v1/constants.h
  schema/v1/schema.h
  schema/v1/types.h
  schema/v1/util.h
  schema/v2/constants.h
  schema/v2/schema.h
  schema/v3/constants.h
  schema/v3/schema.h
  schema/v3/types.h
  schema/v3/util.h
  schema/v4/constants.h
  schema/v4/schema.h
  schema/v4/types.h
  schema/v4/util.h

  MMAI.h
  StdInc.h
  common.h
)

option(ENABLE_MMAI_TEST "Compile tests" OFF)
option(ENABLE_MMAI_STRICT_LOAD "Disable MMAI fallback during model load and throw an error instead" OFF)
option(ENABLE_LIBTORCH "link against libtorch to enable loading of MMAI_MODEL TorchJIT files" ON)

#[[
About the ENABLE_LIBTORCH flag:

VCMI uses a lot of 3rd party libs (like sdl, boost, zlib, etc.) which are
installed by Linux package managers (e.g. `apt install libsdl2-dev`).
Those libs are all pre-compiled with CXX11 ABI. VCMI itself is also compiled
with CXX11 ABI, so it can link to those libs.

For ML, there is an additional 3rd party lib needed: libtorch. However, it is
not installed by a Linux package manager, but by Python's `pip` tool instead.
Unfortunately, it is pre-compiled *without* CXX11 ABI, so VCMI linking fails.

There are two solutions to this issue:

1. Compile `libtorch` from source with CXX11 ABI.
    This is not a straight-forward operation.
    If CUDA support is required, it becomes tricky as graphics card drivers
    may also need to be compiled from source.
    There is a pre-compiled `libtorch` CXX11 ABI binary, but it has no CUDA support.
    (see https://github.com/pytorch/pytorch/issues/51039#issuecomment-2111849591)
2. Compile `VCMI` from source without CXX11 ABI.
    This is also not a straight-forward operation.
    It requires compiling all dependencies from scratch (sdl, boost, etc.).
    If using a C++ package manager (like "conan"), it should be possible.
    I was unable to make VCMI deps compile with conan on Linux (Ubuntu 22.04).

Alternative to the above *solutions*, there are two *workarounds* instead:

1. Compile VCMI with `-D ENABLE_LIBTORCH=0`.
* Pros: allows training new models on CPU and GPU
* Cons: prevents loading pre-trained models for gameplay
* Requirements: you have installed the "default" Python torch package with `pip`
  (see `requirements.txt` in vcmi-gym).

2. Compile VCMI with `-D ENABLE_LIBTORCH=1`.
* Pros: allows loading pre-trained models for gameplay
* Cons: prevents training of new models on GPU (can be trained on CPU only).
  Requirements: you have installed the CPU-only cxx11 ABI torch package with `pip`
  (see `requirements.txt` in vcmi-gym).
]]

set(MMAI_LIBS vcmi)
set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_LIBTORCH)
    add_definitions(-DENABLE_LIBTORCH=1)

    # Disable MMAI fallback (to StupidAI or BattleAI) if model cannot be loaded
    if (ENABLE_MMAI_STRICT_LOAD)
      add_definitions(-DENABLE_MMAI_STRICT_LOAD=1)
    endif()

    # If used for ML training with vcmi-gym, make sure this libtorch is
    # the the same version as the `torch` downloaded by pip in the vcmi-gym project
    # You can use `pip show torch` to see where it is located and then pass
    #
    #     -DLIBTORCH_PATH=/path/to/pip/torch
    #
    # If used for ML inference (regular gameplay), then torch must be
    # compiled for the appropriate platform.
    #
    #
    #  1. Download pytorch (best to use a directory outside VCMI)
    #
    #     $ cd /some/path
    #     $ git clone --recursive https://github.com/pytorch/pytorch
    #     $ cd pytorch
    #     $ git checkout v2.4.1
    #     $ git submodule sync
    #     $ git submodule update --init --recursive
    #     $ python3.10 -m venv .venv
    #     $ .venv/bin/activate
    #     $ pip install cmake ninja
    #     $ pip install -r requirements.txt
    #     $ python setup.py develop
    #
    #  2. Build libtorch
    #
    #   2a) For ios or mac
    #
    #     $ scripts/build_ios.sh
    #     $ export LIBTORCH_PATH="$PWD/build_ios/install"
    #
    #   2b) For other platforms
    #
    #     TODO
    #
    #  3. Build VCMI with -DENABLE_LIBTORCH=1 -DLIBTORCH_PATH=$LIBTORCH_PATH
    #

    if(NOT DEFINED LIBTORCH_PATH OR LIBTORCH_PATH STREQUAL "")
      set(LIBTORCH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libtorch")
      message(FATAL_ERROR "LIBTORCH_PATH is not set")
    endif()

    message(STATUS "LIBTORCH_PATH: ${LIBTORCH_PATH}")

    list(APPEND CMAKE_PREFIX_PATH "${LIBTORCH_PATH}")
    find_package(Torch REQUIRED)

    if(APPLE_IOS)
      # Found in system SDK, somewhere in `xcrun --sdk iphoneos --show-sdk-path`
      find_library(ACCELERATE Accelerate)
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

    list(APPEND MMAI_LIBS "${TORCH_LIBRARIES}" "${ACCELERATE}")
    list(APPEND MMAI_FILES BAI/model/TorchModel.cpp)
    list(APPEND MMAI_INCLUDES "${TORCH_INCLUDE_DIRS}")

    message("MMAI_LIBS: ${MMAI_LIBS}")
    message("MMAI_FILES: ${MMAI_INCLUDES}")
    message("MMAI_INCLUDES: ${MMAI_INCLUDES}")
    # message(STATUS "TORCH_INCLUDE_DIRS: ${TORCH_INCLUDE_DIRS}")
    # message(STATUS "TORCH_CXX_FLAGS: ${TORCH_CXX_FLAGS}")
    # message(STATUS "TORCH_LIBRARIES: ${TORCH_LIBRARIES}")
else()
    list(APPEND MMAI_FILES BAI/model/TorchModelDummy.cpp)
endif()

if(NOT ENABLE_STATIC_LIBS)
  list(APPEND MMAI_FILES main.cpp StdInc.cpp)
endif()
assign_source_group(${MMAI_FILES})

# The VCPKG tool monkey-patches "add_library()" and "add_executable()" to also copy all linked libs during build.
# A modified version of VCPKG's script is used for MMAI.dll to copy its linked libs to the VCMI exe dir.
# (this allows to run VCMI directly from build dir, without needing to install it)
# XXX: the "install()" part is not actually implemented here, so this function should be used for debug purposes only
#      (i.e. to reveal which libs are needed)
function(autoinstall_torch_dlls)
  add_custom_command(TARGET MMAI POST_BUILD
      COMMAND ${Z_VCPKG_POWERSHELL_PATH} -noprofile -executionpolicy Bypass -file ${CMAKE_CURRENT_SOURCE_DIR}/dllcp.ps1
          -targetBinary $<TARGET_FILE:MMAI>
          -originDir "${LIBTORCH_PATH}/lib"
          -destinationDir $<TARGET_FILE_DIR:vcmi> #-Verbose
      VERBATIM
  )
endfunction(autoinstall_torch_dlls)

function(install_torch_libs)
  foreach(libname IN LISTS ARGN)
    unset(${libname}_LIBRARY CACHE)
    find_library(${libname}_LIBRARY ${libname} PATHS "${TORCH_INSTALL_PREFIX}/lib")
    set(libpath ${${libname}_LIBRARY})

    message("Found ${libname}: ${libpath}")
    install(FILES ${libpath} DESTINATION ${BIN_DIR})

    if(MSVC)
      add_custom_command(TARGET MMAI POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libpath}" "$<TARGET_FILE_DIR:vcmi>")
    endif()
  endforeach()
endfunction(install_torch_libs)

if(ENABLE_STATIC_LIBS)
  add_library(MMAI STATIC ${MMAI_FILES})
else()
  add_library(MMAI SHARED ${MMAI_FILES})
  install(TARGETS MMAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})

  if(ANDROID)
    # libtorch is static
  elseif(APPLE_IOS)
    # libtorch is static
  elseif(APPLE_MACOS)
    install_torch_libs(torch_cpu c10)
  elseif(MINGW)
    # TODO
    message(FATAL_ERROR "TODO: install torch libs on MINGW")
  elseif(MSVC)
    install_torch_libs(torch_cpu c10 fbgemm asmjit)
  else(MSVC)
    # TODO
    message(FATAL_ERROR "TODO: install torch libs on XDG")
  endif()
endif()

target_link_libraries(MMAI ${MMAI_LIBS})
set_target_properties(MMAI PROPERTIES COMPILE_DEFINITIONS "MMAI_DLL=1")
target_include_directories(MMAI PUBLIC ${MMAI_INCLUDES})

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
