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
set(MMAI_LIBTORCH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libtorch" CACHE PATH "Link against libtorch located at the specified path (don't link if empty)")

#[[
About the MMAI_LIBTORCH_PATH flag:

MMAI_LIBTORCH_PATH must point to a lightweight (CPU-only) version of "libtorch"
which will be used by VCMI for loading pre-trained ML models during gameplay.
If MMAI_LIBTORCH_PATH is empty, the cmake will throw an error unless the ENABLE_ML
flag is also set, since during model training, a *different* libtorch is
already loaded by vcmi-gym.

TL; DR
Depending on what VCMI is being compiled for:
a) gameplay (i.e. if ENABLE_ML=0): MMAI_LIBTORCH_PATH must be a valid path
b) training (i.e. if ENABLE_ML=1): MMAI_LIBTORCH_PATH must be blank

PS. It is possible to have both set, effectively using the same "libtorch"
in VCMI and vcmi-gym, but there are drawbacks to this approach (GPU cannot
be used for training). More details in vcmi-gym's `requirements.txt`.
]]

message(STATUS "MMAI_LIBTORCH_PATH: ${MMAI_LIBTORCH_PATH}")

set(MMAI_LIBS vcmi)
set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

if(MMAI_LIBTORCH_PATH)
  add_definitions(-DUSING_LIBTORCH=1)
  if (ENABLE_MMAI_STRICT_LOAD)
    add_definitions(-DENABLE_MMAI_STRICT_LOAD=1)
  endif()

  list(APPEND CMAKE_PREFIX_PATH "${MMAI_LIBTORCH_PATH}")
  find_package(Torch REQUIRED)

  if(APPLE_IOS)
    # Found in system SDK, somewhere in `xcrun --sdk iphoneos --show-sdk-path`
    find_library(ACCELERATE Accelerate)
    list(APPEND MMAI_LIBS "${ACCELERATE}")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

  list(APPEND MMAI_LIBS "${TORCH_LIBRARIES}")
  list(APPEND MMAI_FILES BAI/model/TorchModel.cpp)
  list(APPEND MMAI_INCLUDES "${TORCH_INCLUDE_DIRS}")

  message("MMAI_LIBS: ${MMAI_LIBS}")
  message("MMAI_FILES: ${MMAI_INCLUDES}")
  message("MMAI_INCLUDES: ${MMAI_INCLUDES}")
  # message(STATUS "TORCH_INCLUDE_DIRS: ${TORCH_INCLUDE_DIRS}")
  # message(STATUS "TORCH_CXX_FLAGS: ${TORCH_CXX_FLAGS}")
  # message(STATUS "TORCH_LIBRARIES: ${TORCH_LIBRARIES}")
else()
  if(NOT ENABLE_ML)
    # at least one of them must be set
    message(FATAL_ERROR "A blank MMAI_LIBTORCH_PATH is allowed only when ENABLE_ML is set")
  endif()

  list(APPEND MMAI_FILES BAI/model/TorchModelDummy.cpp)
endif()

if(NOT ENABLE_STATIC_LIBS)
  list(APPEND MMAI_FILES main.cpp StdInc.cpp)
endif()
assign_source_group(${MMAI_FILES})

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
    install(FILES ${libpath} DESTINATION ${BIN_DIR})
  endforeach()
endfunction(install_torch_libs)

if(ENABLE_STATIC_LIBS)
  add_library(MMAI STATIC ${MMAI_FILES})
else()
  add_library(MMAI SHARED ${MMAI_FILES})
  install(TARGETS MMAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()

if(MINGW)
  # MinGW libtorch is not supported (see https://stackoverflow.com/a/76160277)
  message(FATAL_ERROR "libtorch is not supported on MinGW systems")
elseif(APPLE_IOS OR ANDROID)
  # mobile libtorch libs are static, nothing to do
else()
  install_torch_libs(torch torch_cpu c10)
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
