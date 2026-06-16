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
# 注：fmt 8.1.1 的 consteval 格式串检查曾在 clang21 报错，故 Step 1 临时加过
# FMT_USE_CONSTEVAL=0。升级到 fmt 10.2.1 后该问题消失，已去掉 workaround，
# 恢复 fmt 的编译期格式串检查(clang21 + gcc13 双平台验证通过)。
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

##########################################################################
# 跨平台 rpath 起点：macOS=@loader_path，Linux=$ORIGIN(指向 .so 自身所在目录)。
if(APPLE)
  set(KFV4_RPATH_ORIGIN "@loader_path")
else()
  set(KFV4_RPATH_ORIGIN "$ORIGIN")
endif()

##########################################################################
# Step 2: 精简 pybind11 模块 pykungfu(只 longfist + yijinjing)，链 Step 1 的 libkungfu。
option(KFV4_BUILD_PYTHON "构建最小点亮的 Python 绑定" OFF)
# Step 4: pykungfu 内嵌 libnode(node::Start 进程内启 Node)。需 -DKFV4_LIBNODE_DIR=<已 --shared 编出
# libnode 的 Node 源码树>(含 out/Release/libnode.*.dylib + src/ + deps/{v8,uv}/include)。
option(KFV4_BUILD_LIBNODE "Step 4: pykungfu 内嵌 libnode(单进程三语言联动)" OFF)
if(KFV4_BUILD_PYTHON)
  find_package(pybind11 REQUIRED)
  set(PY_BINDING_DIR ${CORE_DIR}/src/bindings/python/binding)
  set(PYKUNGFU_SOURCES
      ${PY_BINDING_DIR}/py-longfist.cpp
      ${PY_BINDING_DIR}/py-longfist-types.cpp
      ${PY_BINDING_DIR}/py-longfist-enums.cpp
      ${PY_BINDING_DIR}/py-yijinjing.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/python/pykungfu_lightup.cpp)
  if(KFV4_BUILD_LIBNODE)
    list(APPEND PYKUNGFU_SOURCES ${PY_BINDING_DIR}/py-libnode.cpp)
  endif()
  pybind11_add_module(pykungfu ${PYKUNGFU_SOURCES})
  target_include_directories(pykungfu PRIVATE ${PY_BINDING_DIR})
  target_link_libraries(pykungfu PRIVATE ${LIBKUNGFU_NAME})
  if(KFV4_BUILD_LIBNODE)
    if(NOT KFV4_LIBNODE_DIR)
      message(FATAL_ERROR "KFV4_BUILD_LIBNODE=ON 需 -DKFV4_LIBNODE_DIR=<Node 源码树(已 --shared 编出 libnode)>")
    endif()
    if(APPLE)
      file(GLOB KFV4_LIBNODE_DYLIB ${KFV4_LIBNODE_DIR}/out/Release/libnode.*.dylib)
    else()
      file(GLOB KFV4_LIBNODE_DYLIB ${KFV4_LIBNODE_DIR}/out/Release/libnode.so.*)
    endif()
    message(STATUS "libnode: ${KFV4_LIBNODE_DYLIB}")
    target_compile_definitions(pykungfu PRIVATE KFV4_WITH_LIBNODE)
    target_include_directories(pykungfu PRIVATE
        ${KFV4_LIBNODE_DIR}/src ${KFV4_LIBNODE_DIR}/deps/v8/include ${KFV4_LIBNODE_DIR}/deps/uv/include)
    target_link_libraries(pykungfu PRIVATE ${KFV4_LIBNODE_DYLIB})
  endif()
  # pykungfu 与 libkungfu 同在 build/Release/，rpath 用 origin；libnode 另加其目录。
  set_target_properties(pykungfu PROPERTIES
      BUILD_WITH_INSTALL_RPATH ON
      INSTALL_RPATH "${KFV4_RPATH_ORIGIN}")
  if(KFV4_BUILD_LIBNODE)
    set_property(TARGET pykungfu APPEND PROPERTY INSTALL_RPATH ${KFV4_LIBNODE_DIR}/out/Release)
  endif()
endif()

##########################################################################
# Step 3: 精简 N-API addon kungfu_node(只 longfist + journal 读取)，链 Step 1 的 libkungfu。
# 标准 N-API addon，对系统 Node 22 编(不依赖 libnode；进程内嵌 Node 是 Step 4 的事)。
option(KFV4_BUILD_NODE "构建最小点亮的 Node.js 绑定" OFF)
if(KFV4_BUILD_NODE)
  # node-addon-api 头(.v4/node/node_modules，header-only)与系统 node 头(node@22)。
  # include_dir 是相对 cwd 的路径，故指定 cwd 为 .v4/node 后再补成绝对路径。
  execute_process(
      COMMAND node -p "require('node-addon-api').include_dir"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/node
      OUTPUT_VARIABLE NODE_ADDON_API_REL OUTPUT_STRIP_TRAILING_WHITESPACE)
  get_filename_component(NODE_ADDON_API_DIR
      ${CMAKE_CURRENT_SOURCE_DIR}/node/${NODE_ADDON_API_REL} ABSOLUTE)
  execute_process(
      COMMAND node -p "process.execPath"
      OUTPUT_VARIABLE NODE_EXEC_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  get_filename_component(NODE_BIN_DIR ${NODE_EXEC_PATH} DIRECTORY)
  get_filename_component(NODE_PREFIX ${NODE_BIN_DIR} DIRECTORY)
  set(NODE_INCLUDE_DIR ${NODE_PREFIX}/include/node)
  message(STATUS "node-addon-api headers: ${NODE_ADDON_API_DIR}")
  message(STATUS "node headers: ${NODE_INCLUDE_DIR}")

  set(NODE_BINDING_DIR ${CORE_DIR}/src/bindings/node/binding)
  add_library(kungfu_node SHARED
      ${NODE_BINDING_DIR}/longfist.cpp
      ${NODE_BINDING_DIR}/journal.cpp
      ${NODE_BINDING_DIR}/io.cpp
      ${NODE_BINDING_DIR}/operators.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/node/kungfu_node_lightup.cpp)
  set_target_properties(kungfu_node PROPERTIES PREFIX "" SUFFIX ".node")
  target_include_directories(kungfu_node PRIVATE
      ${NODE_BINDING_DIR} ${NODE_ADDON_API_DIR} ${NODE_INCLUDE_DIR})
  target_compile_definitions(kungfu_node PRIVATE NAPI_VERSION=8 NAPI_EXPERIMENTAL)
  target_link_libraries(kungfu_node PRIVATE ${LIBKUNGFU_NAME})
  # N-API 符号在加载时由宿主 node 解析：macOS 需 -undefined dynamic_lookup 放行未定义符号；
  # Linux 共享对象默认即允许未定义符号，无需额外 flag。
  if(APPLE)
    target_link_options(kungfu_node PRIVATE -undefined dynamic_lookup)
  endif()
  set_target_properties(kungfu_node PROPERTIES
      BUILD_WITH_INSTALL_RPATH ON
      INSTALL_RPATH "${KFV4_RPATH_ORIGIN}")
endif()
