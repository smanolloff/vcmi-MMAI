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
    install(FILES ${libpath} DESTINATION ${LIB_DIR})
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
