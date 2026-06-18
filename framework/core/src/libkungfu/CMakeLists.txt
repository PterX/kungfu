project(${LIBKUNGFU_NAME})

enable_windows_export_all_symbols()

file(GLOB_RECURSE KUNGFU_OPTIM_SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE KUNGFU_CACHE_SOURCE_FILES ${PROJECT_SOURCE_DIR}/yijinjing/cache/*.cpp)
list(REMOVE_ITEM KUNGFU_OPTIM_SOURCE_FILES ${KUNGFU_CACHE_SOURCE_FILES})

add_library_object(kungfu_cache "${KUNGFU_CACHE_SOURCE_FILES}" "${COMPILER_OPTIMIZE_OFF_OPTIONS}" "${KUNGFU_BUILD_DIR}")
add_library_object(kungfu_optim "${KUNGFU_OPTIM_SOURCE_FILES}" "${COMPILER_OPTIMIZE_ON_OPTIONS}" "${KUNGFU_BUILD_DIR}")

# Windows: 用 STATIC。libkungfu 含 rocksdb/boost::hana/sqlite_orm 大量模板实例化，作为 SHARED DLL 时
# CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 自动导出的符号数 >65535，撞 COFF 库成员上限(LNK1189)。改 STATIC 由各
# binding 链接；journal 是 mmap 文件、跨实例本就共享，故 Windows 上多 binding 各自静态链接 libkungfu 仍读同一 journal。
# Mac/Linux 维持 SHARED(运行时多语言绑定在同进程共享同一 dylib/so)。
if (WIN32)
  add_library(${LIBKUNGFU_NAME} STATIC $<TARGET_OBJECTS:kungfu_optim> $<TARGET_OBJECTS:kungfu_cache>)
else ()
  add_library(${LIBKUNGFU_NAME} SHARED $<TARGET_OBJECTS:kungfu_optim> $<TARGET_OBJECTS:kungfu_cache>)
endif ()
set_target_properties(${LIBKUNGFU_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${LIBKUNGFU_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${KUNGFU_BUILD_DIR})
target_compile_options(${LIBKUNGFU_NAME} PRIVATE "$<$<CONFIG:Release>:${COMPILER_OPTIMIZE_ON_OPTIONS}>")
target_link_libraries(${LIBKUNGFU_NAME} ${CONAN_LIBS})
