// SPDX-License-Identifier: Apache-2.0
//
// 开放层运行时投影器（生产 reader seam）：把 open-layer FB 帧（msg_type 不在 longfist 闭集）
// 按 .bfbs 运行时反射投影到独立 sqlite db。挂在 `cached::feed()` 上，与 hana×sqlite_orm 闭集**并存**：
//   - feed(event) 只处理已注册的 open-layer msg_type，其余帧一律 no-op（交给 hana 路径），互不干扰；
//   - 默认 OFF：cached 仅在环境变量 KF_OPEN_LAYER_SCHEMAS 指向 schemas 目录时才启用本投影器。
// schemas 来源（本切片口径）：KF_OPEN_LAYER_SCHEMAS 目录下 `manifest.txt`，每行 `msg_type table bfbs_file`
//   （`#` 开头为注释），逐行 own .bfbs 字节注册进 SchemaRegistry。真正配置格式待后续定。
// 生命周期：own SchemaRegistry(own .bfbs 字节) + own sqlite3 db；非线程安全，由 cached 在 feed_mutex_ 下调用。
#ifndef KUNGFU_YIJINJING_CACHE_OPEN_LAYER_PROJECTOR_H
#define KUNGFU_YIJINJING_CACHE_OPEN_LAYER_PROJECTOR_H

#include <kungfu/common.h>
#include <kungfu/yijinjing/cache/fb_schema_registry.h>

#include <flatbuffers/util.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace kungfu::yijinjing::cache {

class open_layer_projector {
public:
  open_layer_projector() = default;
  ~open_layer_projector() {
    if (db_ != nullptr)
      sqlite3_close(db_);
  }
  open_layer_projector(const open_layer_projector &) = delete;
  open_layer_projector &operator=(const open_layer_projector &) = delete;

  // 从 schemas_dir/manifest.txt 注册全部开放层类型，open db_path、reconcile 建表。返回注册类型数。
  size_t setup(const std::string &schemas_dir, const std::string &db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK)
      throw std::runtime_error("open_layer_projector: cannot open db " + db_path);
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

    const std::string manifest = schemas_dir + "/manifest.txt";
    std::ifstream mf(manifest);
    if (!mf)
      throw std::runtime_error("open_layer_projector: no manifest at " + manifest);
    std::string line;
    while (std::getline(mf, line)) {
      size_t b = line.find_first_not_of(" \t\r\n");
      if (b == std::string::npos || line[b] == '#')
        continue; // 空行 / 注释
      std::istringstream iss(line.substr(b));
      int32_t msg_type = 0;
      std::string table, bfbs_file;
      if (!(iss >> msg_type >> table >> bfbs_file))
        continue;
      std::string bfbs;
      if (!flatbuffers::LoadFile((schemas_dir + "/" + bfbs_file).c_str(), /*binary*/ true, &bfbs))
        throw std::runtime_error("open_layer_projector: cannot read bfbs " + bfbs_file);
      registry_.add(msg_type, table, std::move(bfbs), /*thin*/ true);
    }
    registry_.reconcile_all(db_);
    return registry_.size();
  }

  // 投影一帧：已注册 msg_type -> 零拷贝反射投影入库；未注册 -> no-op（留给 hana 路径）。返回是否投影。
  bool feed(const event_ptr &event) {
    const auto *entry = registry_.find(event->msg_type());
    if (entry == nullptr)
      return false;
    projector::project_frame(db_, *entry, reinterpret_cast<const uint8_t *>(event->data_address()), event->gen_time(),
                             event->frame_uid(), event->stream_id());
    return true;
  }

  [[nodiscard]] const projector::SchemaRegistry &registry() const { return registry_; }
  [[nodiscard]] sqlite3 *db() const { return db_; }

private:
  projector::SchemaRegistry registry_;
  sqlite3 *db_ = nullptr;
};

DECLARE_PTR(open_layer_projector)
} // namespace kungfu::yijinjing::cache

#endif // KUNGFU_YIJINJING_CACHE_OPEN_LAYER_PROJECTOR_H
