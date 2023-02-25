find_path(KUNGFUDEPS_INCLUDE_DIR NAMES kungfu/common.h PATHS ${CONAN_INCLUDE_DIRS_KUNGFU-CORE})
find_library(KUNGFUDEPS_LIBRARY NAMES ${CONAN_LIBS_KUNGFU-CORE} PATHS ${CONAN_LIB_DIRS_KUNGFU-CORE})

include(${CONAN_KUNGFU-CORE_ROOT}/cmake/compiler.cmake)
include_directories(${CONAN_KUNGFU-CORE_ROOT}/deps/hana/include)

SET(KFC_EXECUTABLE "$ENV{KFC_HOME}/kfc")

string(REPLACE "\n" "" KFC_EXECUTABLE ${KFC_EXECUTABLE})
string(REPLACE "\"" "" KFC_EXECUTABLE ${KFC_EXECUTABLE})

set(PYTHON_EXECUTABLE ${KFC_EXECUTABLE})
set(ENV{KFC_AS_VARIANT} python)

macro(find_pybind11)
  if (NOT TARGET pybind11::module)
    add_subdirectory(${CONAN_KUNGFU-CORE_ROOT}/deps/pybind11 "${PROJECT_BINARY_DIR}/pybind11")
  endif ()
endmacro()

macro(kungfu_extension MODULE_NAME)
  add_compile_definitions(KUNGFU_MODULE_NAME=${MODULE_NAME})
  add_compile_definitions(FMT_HEADER_ONLY)
  add_compile_definitions(SPDLOG_ACTIVE_LEVEL=0)
  add_compile_definitions(SPDLOG_NO_NAME)
  add_compile_definitions(SPDLOG_NO_ATOMIC_LEVELS)
  
  message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
  
  if (${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    set(CMAKE_CXX_FLAGS_RELEASE "-O")
  endif ()

  set(SRC_DIR ${ARGN})
  list(LENGTH SRC_DIR extra_args_count)

  if (${extra_args_count} EQUAL 0)
    message(STATUS "SRC_DIR is empty, use default src")
    aux_source_directory("src" SOURCE_0)
  else()
    message(STATUS "SRC_DIR is not empty, use ${SRC_DIR}")
    aux_source_directory(${SRC_DIR} SOURCE_0)
  endif()
  set(SOURCES  ${SOURCE_0})

  set(BUILD_OUTPUT_DIR "${PROJECT_BINARY_DIR}/target")

  find_pybind11()

  pybind11_add_module(${MODULE_NAME} SHARED ${SOURCES})
  target_link_libraries(${MODULE_NAME} PRIVATE kungfu ${CONAN_LIBS})
  set_target_properties(${MODULE_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR})
  set_target_properties(${MODULE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BUILD_OUTPUT_DIR})
  set_target_properties(${MODULE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BUILD_OUTPUT_DIR})
endmacro()

set(KUNGFUDEPS_FOUND TRUE)
set(KUNGFUDEPS_INCLUDE_DIRS ${KUNGFUDEPS_INCLUDE_DIR})
set(KUNGFUDEPS_LIBRARIES ${KUNGFUDEPS_LIBRARY})
mark_as_advanced(KUNGFUDEPS_LIBRARY KUNGFUDEPS_INCLUDE_DIR)
