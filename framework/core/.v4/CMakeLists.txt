# Kungfu 4.0 — 最小点亮内核驱动（conan 2 + 直接 cmake）
# 复用既有 .cmake/compiler.cmake 与 src/libkungfu/CMakeLists.txt，
# 用 conan 2 CMakeDeps 的 find_package 目标取代 conan 1.x 的 ${CONAN_LIBS}。
# 仅编 libkungfu(longfist + yijinjing)；绑定/打包不在此驱动内。
cmake_minimum_required(VERSION 3.20)
project(kungfu_v4_kernel CXX)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT DEFINED SPDLOG_LOG_LEVEL_COMPILE)
  set(SPDLOG_LOG_LEVEL_COMPILE SPDLOG_LEVEL_TRACE)
endif()

include(${CORE_DIR}/.cmake/compiler.cmake)

##########################################################################
# conan 2 依赖（由 CMakeToolchain 工具链 + CMakeDeps 提供）
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(nng REQUIRED)
find_package(rxcpp REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(RocksDB REQUIRED)
find_package(tabulate REQUIRED)

set(CONAN_LIBS
    fmt::fmt-header-only
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    nng::nng
    rxcpp::rxcpp
    SQLite::SQLite3
    RocksDB::rocksdb
    tabulate::tabulate)

##########################################################################
add_compile_definitions(HAVE_USLEEP=1)
# fmt 8.1.1 的 consteval 格式串检查在 clang 21 上报错；关掉编译期检查以最小代价点亮。
# TODO(4.0): 后续把 fmt/spdlog 升到现代版本，去掉此 workaround。
add_compile_definitions(FMT_USE_CONSTEVAL=0)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_LOG_LEVEL_COMPILE})
add_compile_definitions(SPDLOG_NO_NAME)
add_compile_definitions(SPDLOG_NO_ATOMIC_LEVELS)

option(NNG_TESTS "" OFF)
option(NNG_TOOLS "" OFF)
option(NNG_ENABLE_NNGCAT "" OFF)

##########################################################################
include_directories(${CORE_DIR}/.deps/hana-1.80.0/include)
include_directories(${CORE_DIR}/.deps/sqlite_orm-1.7.1/include)
include_directories(${CORE_DIR}/src/include)

##########################################################################
if(DEFINED ENV{LIBKUNGFU_NAME})
  set(LIBKUNGFU_NAME $ENV{LIBKUNGFU_NAME})
else()
  set(LIBKUNGFU_NAME kungfu)
endif()

set(KUNGFU_BUILD_DIR ${CMAKE_BINARY_DIR})
set(KUNGFU_BUILD_LIBRARY_DIR ${KUNGFU_BUILD_DIR}/${CMAKE_BUILD_TYPE})
link_directories(${KUNGFU_BUILD_LIBRARY_DIR})

##########################################################################
# conan 2 用 target usage-requirement 传播 include；libkungfu 走 OBJECT 库分别编译，
# 需在定义 target 前用 link_libraries 全局挂上依赖，复刻旧 conan_basic_setup 的全局 include 行为。
link_libraries(${CONAN_LIBS})

add_subdirectory(${CORE_DIR}/src/libkungfu ${CMAKE_BINARY_DIR}/libkungfu)
