project(kungfu-python-binding)

set(KF_PYTHON_BINDING_NAME pykungfu)
message(STATUS "Configuring for python binding ${KF_PYTHON_BINDING_NAME}")

use_libnode()

aux_source_directory(binding SOURCE_FILES_PYKUNGFU)
# tracing-foundation Phase 1: wingchun 移出编译流程,排除 py-wingchun-* 绑定源(py-longfist 保留,交易类型仍定义于 types.h)。
list(FILTER SOURCE_FILES_PYKUNGFU EXCLUDE REGEX "py-wingchun")
pybind11_add_module(${KF_PYTHON_BINDING_NAME} SHARED ${SOURCE_FILES_PYKUNGFU})
target_link_libraries(${KF_PYTHON_BINDING_NAME} PRIVATE ${LIBNODE} kungfu)
