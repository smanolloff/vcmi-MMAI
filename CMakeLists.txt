set(MMAI_FILES
  AAI/AAI.cpp
  AAI/AAI.h

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
  $ . ./venv/bin/activate
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

  set(EXECUTORCH_BUILD_EXTENSION_FLAT_TENSOR  ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_DATA_LOADER  ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_MODULE       ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_EXTENSION_TENSOR       ON CACHE BOOL "" FORCE)
  set(EXECUTORCH_BUILD_XNNPACK                ON CACHE BOOL "" FORCE)

  set(EXECUTORCH_BUILD_TESTING                OFF CACHE BOOL "" FORCE)
  set(EXECUTORCH_ENABLE_PYBIND                OFF CACHE BOOL "" FORCE)

  # keep this OFF (improves performance)
  set(EXECUTORCH_OPTIMIZE_SIZE                OFF CACHE BOOL "" FORCE)

  # XXX: This does not work (overwritten by XNNPACK based on CMAKE_BUILD_TYPE)
  set(XNN_LOG_LEVEL 2 CACHE STRING "" FORCE)

  set(PYTHON_EXECUTABLE "${MMAI_EXECUTORCH_PATH}/.venv/bin/python" CACHE FILEPATH "" FORCE)
  message("PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")

  add_subdirectory("${MMAI_EXECUTORCH_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/executorch")

  list(APPEND MMAI_FILES BAI/model/TorchModel.cpp)
  list(APPEND MMAI_LIBS
    executorch
    xnnpack_backend
    portable_ops_lib
    extension_module_static
    extension_flat_tensor
    extension_tensor
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
