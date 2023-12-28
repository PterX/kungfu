//
// Created by PC on 2023/12/28.
//
#include <kungfu/yijinjing/util/rocks.h>
namespace kungfu::yijinjing::util {

rocksdb::Status rocks::open_db(const std::string &dir, rocksdb::DB **db, bool is_writing) {
  SPDLOG_DEBUG("open_db: {}", dir);
  if (is_writing) {
    SPDLOG_DEBUG("Open before");
    auto status = rocksdb::DB::Open(rocks::options(), dir, db);
    SPDLOG_DEBUG("Open after ");
    return status;
  } else {
    SPDLOG_DEBUG("OpenForReadOnly before");
    auto status = rocksdb::DB::OpenForReadOnly(rocks::options(), dir, db);
    SPDLOG_DEBUG("OpenForReadOnly after");
    return status;
  }
}

rocks::rocks() {
  read_options_.fill_cache = false;
  options_.create_if_missing = true;
}

rocks_ptr rocks::get_rocks() {
  static rocks_ptr rocks_ = std::shared_ptr<rocks>(new rocks());
  return rocks_;
}

rocksdb::ReadOptions &rocks::read_options() { return get_rocks()->read_options_; }

rocksdb::WriteOptions &rocks::write_options() { return get_rocks()->write_options_; }

rocksdb::Options &rocks::options() {
  SPDLOG_DEBUG("option");
  return get_rocks()->options_;
}

rocksdb::Status rocks::put_kv(const std::string &key, const std::string &value, const std::string &dir) {
  rocksdb::DB *db;
  open_db(dir, &db, true);
  rocksdb::Status status = put_kv(key, value, db);
  delete db;
  return status;
}

rocksdb::Status rocks::put_kv(const std::string &key, const std::string &value, rocksdb::DB *db) {
  try {
    return db->Put(write_options(), key, value);
  } catch (const std::exception &e) {
    SPDLOG_WARN("catch exception: {}", e.what());
  }
  return rocksdb::Status{};
}

rocksdb::Status rocks::put_kvs(const std::map<std::string, std::string> &kvs, const std::string &dir) {
  rocksdb::DB *db;
  open_db(dir, &db, true);
  rocksdb::Status status = put_kvs(kvs, db);
  delete db;
  return status;
}

rocksdb::Status rocks::put_kvs(const std::map<std::string, std::string> &kvs, rocksdb::DB *db) {
  try {
    rocksdb::WriteBatch batch;
    for (const auto &pair : kvs) {
      batch.Put(pair.first, pair.second);
    }
    return db->Write(write_options(), &batch);
  } catch (const std::exception &e) {
    SPDLOG_WARN("catch exception: {}", e.what());
  }
  return rocksdb::Status{};
}

rocksdb::Status rocks::put_kvs(rocksdb::WriteBatch &batch, const std::string &dir) {
  rocksdb::DB *db;
  open_db(dir, &db, true);
  rocksdb::Status status = put_kvs(batch, db);
  delete db;
  return status;
}

rocksdb::Status rocks::put_kvs(rocksdb::WriteBatch &batch, rocksdb::DB *db) {
  try {
    return db->Write(write_options(), &batch);
  } catch (const std::exception &e) {
    SPDLOG_WARN("catch exception: {}", e.what());
  }
  return rocksdb::Status{};
}

rocksdb::Status rocks::get_kv(const std::string &key, std::string &value, const std::string &dir) {
  rocksdb::DB *db;
  open_db(dir, &db, false);
  rocksdb::Status status = get_kv(key, value, db);
  delete db;
  return status;
}

rocksdb::Status rocks::get_kv(const std::string &key, std::string &value, rocksdb::DB *db) {
  try {
    return db->Get(read_options(), key, &value);
  } catch (const std::exception &e) {
    SPDLOG_WARN("catch exception: {}", e.what());
  }
  return rocksdb::Status{};
}

std::map<std::string, std::string> rocks::get_kvs(const std::set<std::string> &keys, const std::string &dir) {
  rocksdb::DB *db;
  open_db(dir, &db, false);
  std::map<std::string, std::string> result = get_kvs(keys, db);
  delete db;
  return result;
}

std::map<std::string, std::string> rocks::get_kvs(const std::set<std::string> &keys, rocksdb::DB *db) {
  std::map<std::string, std::string> result;
  try {
    for (const std::string &key : keys) {
      db->Get(read_options(), key, &result.try_emplace(key).first->second);
    }
  } catch (const std::exception &e) {
    SPDLOG_WARN("catch exception: {}", e.what());
  }
  return result;
}

} // namespace kungfu::yijinjing::util
