# SPDX-License-Identifier: Apache-2.0

set(CMAKE_CXX_STANDARD 20)

############################################################

# ccache：可用即作为编译器 launcher（命中编译缓存，显著加速 clean-rebuild 下
# libkungfu 重模板重编）。零强制耦合：仅在 UNIX 且 PATH 有 ccache 时启用，
# 开源克隆 / Windows / 未装 ccache 的机器 find_program 未命中即 no-op。
# libkungfu(conan build) 与 node/electron bindings(cmake-js) 共用本文件，故一处全覆盖。
if (UNIX)
  find_program(CCACHE_PROGRAM ccache)
  if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "ccache enabled as compiler launcher: ${CCACHE_PROGRAM}")
  endif ()
endif ()

############################################################

# Set the global compile options.
# Some of the options may be override by target_compiles_options later in sub-projects.
if (UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC") # set -fPIC for nng
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-O0")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(COMPILER_OPTIMIZE_ON_OPTIONS "-O3")
  set(COMPILER_OPTIMIZE_OFF_OPTIONS "-O0")
endif ()
if (UNIX AND NOT APPLE)
  set(KFC_INSTALL_RPATH
      "$ORIGIN"
      "$ORIGIN/../../"
      )
  set(CMAKE_INSTALL_RPATH "${KFC_INSTALL_RPATH}")
endif ()
if (APPLE)
  # @executable_path = "Kungfu.app/Contents/Frameworks/Kungfu Helper.app/Contents/MacOS"
  set(KFC_INSTALL_RPATH
      "@loader_path"
      "@loader_path/../../"
      "@executable_path/../../../../Resources/kfc"
      )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-unqualified-std-cast-call -Wno-unused-value")
  set(CMAKE_INSTALL_RPATH "${KFC_INSTALL_RPATH}")
  set(CMAKE_MACOSX_RPATH ON)
  set(CONAN_DISABLE_CHECK_COMPILER ON)
endif ()
if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /utf-8 /permissive- /bigobj /W0 /Zc:__cplusplus")
  message(STATUS "CMAKE_CXX_FLAGS set to ${CMAKE_CXX_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /IGNORE:4199")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /IGNORE:4199")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  add_compile_definitions(HAVE_SNPRINTF)
  add_compile_definitions(V8_DEPRECATION_WARNINGS=1)
  add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  set(COMPILER_OPTIMIZE_ON_OPTIONS "/O2")
  set(COMPILER_OPTIMIZE_OFF_OPTIONS "/Od")
endif ()

if (${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0 -Wno-address-of-packed-member -Wno-deprecated -Wno-nonnull")
endif ()

############################################################

macro(enable_windows_export_all_symbols)
  if (MSVC)
      set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif ()
endmacro()

macro(add_library_object OBJ_NAME SRC_FILES COMPILER_OPTIMIZE_OPTIONS OUTPUT_DIR)
  add_library(${OBJ_NAME} OBJECT ${SRC_FILES})
  set_target_properties(${OBJ_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if (NOT ${OUTPUT_DIR} STREQUAL "")
    set_target_properties(${OBJ_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
  endif ()
  if (NOT ${COMPILER_OPTIMIZE_OPTIONS} STREQUAL "")
    target_compile_options(${OBJ_NAME} PRIVATE $<$<CONFIG:Release>:${COMPILER_OPTIMIZE_OPTIONS}>)
  endif ()
endmacro()
