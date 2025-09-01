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

  BAI/v8/BAI.cpp
  BAI/v8/BAI.h
  BAI/v8/action.cpp
  BAI/v8/action.h
  BAI/v8/attack_log.h
  BAI/v8/battlefield.cpp
  BAI/v8/battlefield.h
  BAI/v8/encoder.cpp
  BAI/v8/encoder.h
  BAI/v8/global_stats.h
  BAI/v8/hex.cpp
  BAI/v8/hex.h
  BAI/v8/hexaction.h
  BAI/v8/hexactmask.h
  BAI/v8/render.cpp
  BAI/v8/render.h
  BAI/v8/stack.cpp
  BAI/v8/stack.h
  BAI/v8/state.cpp
  BAI/v8/state.h
  BAI/v8/supplementary_data.cpp
  BAI/v8/supplementary_data.h
  BAI/v8/util.cpp
  BAI/v8/util.h

  BAI/v9/BAI.cpp
  BAI/v9/BAI.h
  BAI/v9/action.cpp
  BAI/v9/action.h
  BAI/v9/attack_log.h
  BAI/v9/battlefield.cpp
  BAI/v9/battlefield.h
  BAI/v9/encoder.cpp
  BAI/v9/encoder.h
  BAI/v9/global_stats.h
  BAI/v9/hex.cpp
  BAI/v9/hex.h
  BAI/v9/hexaction.h
  BAI/v9/hexactmask.h
  BAI/v9/render.cpp
  BAI/v9/render.h
  BAI/v9/stack.cpp
  BAI/v9/stack.h
  BAI/v9/state.cpp
  BAI/v9/state.h
  BAI/v9/supplementary_data.cpp
  BAI/v9/supplementary_data.h
  BAI/v9/util.cpp
  BAI/v9/util.h

  BAI/v10/BAI.cpp
  BAI/v10/BAI.h
  BAI/v10/action.cpp
  BAI/v10/action.h
  BAI/v10/attack_log.h
  BAI/v10/battlefield.cpp
  BAI/v10/battlefield.h
  BAI/v10/encoder.cpp
  BAI/v10/encoder.h
  BAI/v10/global_stats.cpp
  BAI/v10/global_stats.h
  BAI/v10/hex.cpp
  BAI/v10/hex.h
  BAI/v10/hexaction.h
  BAI/v10/hexactmask.h
  BAI/v10/player_stats.cpp
  BAI/v10/player_stats.h
  BAI/v10/render.cpp
  BAI/v10/render.h
  BAI/v10/stack.cpp
  BAI/v10/stack.h
  BAI/v10/state.cpp
  BAI/v10/state.h
  BAI/v10/supplementary_data.cpp
  BAI/v10/supplementary_data.h
  BAI/v10/util.cpp
  BAI/v10/util.h

  BAI/v11/BAI.cpp
  BAI/v11/BAI.h
  BAI/v11/action.cpp
  BAI/v11/action.h
  BAI/v11/attack_log.h
  BAI/v11/battlefield.cpp
  BAI/v11/battlefield.h
  BAI/v11/encoder.cpp
  BAI/v11/encoder.h
  BAI/v11/global_stats.cpp
  BAI/v11/global_stats.h
  BAI/v11/hex.cpp
  BAI/v11/hex.h
  BAI/v11/hexaction.h
  BAI/v11/hexactmask.h
  BAI/v11/player_stats.cpp
  BAI/v11/player_stats.h
  BAI/v11/render.cpp
  BAI/v11/render.h
  BAI/v11/stack.cpp
  BAI/v11/stack.h
  BAI/v11/state.cpp
  BAI/v11/state.h
  BAI/v11/supplementary_data.cpp
  BAI/v11/supplementary_data.h
  BAI/v11/util.cpp
  BAI/v11/util.h

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
  schema/v3/constants.h
  schema/v3/schema.h
  schema/v3/types.h
  schema/v3/util.h
  schema/v8/constants.h
  schema/v8/schema.h
  schema/v8/types.h
  schema/v8/util.h
  schema/v9/constants.h
  schema/v9/schema.h
  schema/v9/types.h
  schema/v9/util.h
  schema/v10/constants.h
  schema/v10/schema.h
  schema/v10/types.h
  schema/v10/util.h
  schema/v11/constants.h
  schema/v11/schema.h
  schema/v11/types.h
  schema/v11/util.h
  schema/v12/constants.h
  schema/v12/expbin.h
  schema/v12/linbin.h
  schema/v12/schema.h
  schema/v12/types.h
  schema/v12/util.h


  MMAI.h
  StdInc.h
  common.h
)

option(ENABLE_MMAI_TEST "Compile tests" OFF)
option(ENABLE_MMAI_STRICT_LOAD "Disable MMAI fallback during model load and throw an error instead" OFF)
set(MMAI_EXECUTORCH_PATH "" CACHE PATH "Path to executorch v0.7.0 source directory")

#[[
About the MMAI_EXECUTORCH_PATH flag:

`executorch` is the library which handles loading pre-trained ML models during
gameplay. It is compiled as part of VCMI, its source code is external to VCMI.
The executorch repo is intentionally not added as a git submodule of MMAI
since it is huge and actually makes every git op within the VCMI repo slower.

MMAI_EXECUTORCH_PATH is required unless the ENABLE_ML build flag is set
(if ENABLE_ML is set, models loading is handled by the vcmi-gym project).

Example setup:

  $ cd /some/path
  $ git clone --recurse-submodules -b release/0.7 https://github.com/pytorch/executorch.git
  $ cd executorch
  $ python3 -m venv .venv
  $ . ./venv/bin/activate.sh
  $ ./install_requirements.sh

Now you can build VCMI with MMAI_EXECUTORCH_PATH=/some/path/executorch.

]]

set(MMAI_LIBS vcmi)
set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/schema/gcem)

if(MMAI_EXECUTORCH_PATH)
  if(NOT EXISTS "${MMAI_EXECUTORCH_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "No CMakeLists.txt found under MMAI_EXECUTORCH_PATH: ${MMAI_EXECUTORCH_PATH}")
  endif()

  add_definitions(-DUSING_EXECUTORCH=1)

  if (ENABLE_MMAI_STRICT_LOAD)
    add_definitions(-DENABLE_MMAI_STRICT_LOAD=1)
  endif()

  set(EXECUTORCH_ROOT "/Users/simo/Projects/executorch")
  set(EXECUTORCH_BUILD "${EXECUTORCH_ROOT}/build")

  set(EXECUTORCH_OPTIMIZE_SIZE                ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_FLAT_TENSOR  ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_DATA_LOADER  ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_MODULE       ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_TENSOR       ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_XNNPACK                ON CACHE BOOL "" FORCE)

  # XXX: This does not work (overwritten by XNNPACK based on CMAKE_BUILD_TYPE)
  set(XNN_LOG_LEVEL 2 CACHE STRING "" FORCE)

  set(PYTHON_EXECUTABLE "${EXECUTORCH_ROOT}/.venv/bin/python" CACHE FILEPATH "" FORCE)
  message("PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")

  add_subdirectory("${EXECUTORCH_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/executorch")

  list(APPEND MMAI_FILES BAI/model/TorchModel.cpp)

  list(APPEND MMAI_LIBS
    executorch
    extension_module_static
    extension_flat_tensor
    extension_tensor
    # optimized_native_cpu_ops_lib
    xnnpack_backend
    portable_ops_lib  # kernel-ops library that registers all ops.
  )
else()
  if(NOT ENABLE_ML)
    message(FATAL_ERROR "MMAI_EXECUTORCH_PATH is required unless ENABLE_ML is set")
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

target_link_libraries(MMAI PUBLIC ${MMAI_LIBS})
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
