// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-15.
//

#ifndef KUNGFU_HERO_H
#define KUNGFU_HERO_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/index/session.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>
#include <kungfu/yijinjing/util/rocks.h>

#ifndef KUNGFU_SETUP_LOG
#define KUNGFU_SETUP_LOG() kungfu::yijinjing::log::copy_log_settings(get_home(), get_home()->name)
#endif // KUNGFU_SETUP_LOG

namespace kungfu::yijinjing::practice {

inline yijinjing::data::location_ptr make_system_location(const std::string &group, const std::string &name,
                                                          const data::locator_ptr &locator,
                                                          uint32_t seed = KUNGFU_HASH_SEED) {
  return yijinjing::data::location::make_shared(locator->get_dir_mode(), longfist::enums::category::SYSTEM, group, name,
                                                locator, seed);
}

typedef std::unordered_map<uint32_t, yijinjing::journal::writer_ptr> WriterMap;

class hero : public resource {
public:
  explicit hero(yijinjing::io_device_ptr io_device);

  ~hero() override;

  bool is_usable() override;

  virtual void pre_setup();

  bool setup() override;

  void step(uint32_t count = 0);

  void run();

  bool is_live() const;

  bool is_low_latency() const;

  const yijinjing::journal::bus_ptr &get_bus() const;

  void signal_stop();

  int64_t now() const;

  void set_now(int64_t now);

  void set_begin_time(int64_t begin_time);

  int64_t get_begin_time() const;

  void set_end_time(int64_t end_time);

  int64_t get_end_time() const;

  [[nodiscard]] const data::locator_ptr &get_locator() const;

  [[nodiscard]] yijinjing::io_device_ptr get_io_device() const;

  const yijinjing::data::location_ptr &get_home() const;

  uint32_t get_home_uid() const;

  const std::string &get_home_uname() const;

  [[maybe_unused]] const yijinjing::data::location_ptr &get_live_home() const;

  uint32_t get_live_home_uid() const;

  [[maybe_unused]] [[nodiscard]] yijinjing::journal::reader_ptr get_reader() const;

  virtual bool has_writer(uint32_t dest_id) const;

  [[nodiscard]] virtual yijinjing::journal::writer_ptr get_writer(uint32_t dest_id) const;

  bool has_band_writer(uint32_t dest_id) const;

  [[nodiscard]] yijinjing::journal::writer_ptr get_band_writer(uint32_t dest_id) const;

  [[maybe_unused]] [[nodiscard]] const WriterMap &get_writers() const;

  bool has_location(uint32_t uid) const;

  [[nodiscard]] yijinjing::data::location_ptr get_location(uint32_t uid) const;

  [[nodiscard]] std::string get_location_uname(uint32_t uid) const;

  bool is_location_live(uint32_t uid) const;

  bool has_channel(uint32_t source, uint32_t dest) const;

  bool has_channel(uint64_t hash) const;

  [[maybe_unused]] const longfist::types::Channel &get_channel(uint32_t source, uint32_t dest) const;

  const longfist::types::Channel &get_channel(uint64_t hash) const;

  [[maybe_unused]] const std::unordered_map<uint64_t, longfist::types::Channel> &get_channels() const;

  [[maybe_unused]] bool has_band(uint32_t source, uint32_t dest) const;

  bool has_band(uint64_t hash) const;

  [[maybe_unused]] const longfist::types::Band &get_band(uint32_t source, uint32_t dest) const;

  const longfist::types::Band &get_band(uint64_t hash) const;

  [[maybe_unused]] const std::unordered_map<uint64_t, longfist::types::Band> &get_bands() const;

  const std::unordered_map<uint32_t, longfist::types::Register> &get_registry() const;

  const std::unordered_map<uint32_t, yijinjing::data::location_ptr> &get_locations() const;

  virtual void on_notify();

  virtual void on_exit();

  virtual bool is_reactable(const event_ptr &event);

  void ensure_master_rocksdb() const;

  rocksdb::DB *get_master_rocksdb() const;

  rocksdb::DB *get_app_rocksdb() const;

  std::string get_master_kv(const std::string &key) const;

  std::map<std::string, std::string> get_master_kvs(const std::set<std::string> &keys) const;

  void put_master_kv(const std::string &key, const std::string &value) const;

  void put_master_kvs(const std::map<std::string, std::string> &kvs) const;

  std::string get_app_kv(const std::string &key) const;

  std::map<std::string, std::string> get_app_kvs(const std::set<std::string> &keys) const;

  void put_app_kv(const std::string &key, const std::string &value) const;

  void put_app_kvs(const std::map<std::string, std::string> &kvs) const;

  void write_location_to_rocksdb(const data::location_ptr &location);

  void read_location_from_rocksdb();

  void request_deregister() {
    continual_ = false;
    live_ = false;
  }

  yijinjing::data::location_ptr get_ledger_home_location() const;

  yijinjing::data::location_ptr get_master_home_location() const;

  yijinjing::data::location_ptr get_master_cmd_location() const;

  const rx::connectable_observable<event_ptr> &get_events() const;

  void disjoin(uint32_t location_uid);

  void disjoin_channel(uint32_t location_uid, uint32_t dest_id);

  void disjoin_channel(const yijinjing::data::location_ptr &location, uint32_t dest_id);

protected:
  int64_t begin_time_;
  int64_t end_time_;
  yijinjing::journal::reader_ptr reader_;
  WriterMap writers_ = {};
  WriterMap band_writers_ = {};
  mutable std::mutex band_mtx_{};
  const size_t main_thread_id_{};
  std::set<std::string> location_uid64s_ = {};

  rx::connectable_observable<event_ptr> events_ = {};

  const yijinjing::data::location_ptr master_home_location_;
  const yijinjing::data::location_ptr master_cmd_location_;
  const yijinjing::data::location_ptr ledger_home_location_;

  yijinjing::io_device_ptr io_device_;
  volatile bool live_ = false;
  volatile uint32_t step_limit_ = 0;
  int64_t now_;

  static uint64_t make_source_dest_hash(uint32_t source_id, uint32_t dest_id);

  bool check_location_exists(uint32_t source_id, uint32_t dest_id) const;

  bool check_location_live(uint32_t source_id, uint32_t dest_id) const;

  void add_location(int64_t trigger_time, const yijinjing::data::location_ptr &location);

  void add_location(int64_t trigger_time, const longfist::types::Location &location);

  [[maybe_unused]] void remove_location(int64_t trigger_time, uint32_t location_uid);

  void register_location(int64_t trigger_time, const longfist::types::Register &register_data);

  void deregister_location(int64_t trigger_time, uint32_t location_uid);

  void register_channel([[maybe_unused]] int64_t trigger_time, const longfist::types::Channel &channel);

  void deregister_channel(uint32_t source_id);

  void register_band(int64_t trigger_time, const longfist::types::Band &band);

  void deregister_band(uint32_t source_id);

  void require_read_from(int64_t trigger_time, uint32_t dest_id, uint32_t source_id, int64_t from_time,
                         uint64_t page_size = 0);

  void require_read_from_public(int64_t trigger_time, uint32_t dest_id, uint32_t source_id, int64_t from_time,
                                uint64_t page_size = 0);

  void require_read_from_sync(int64_t trigger_time, uint32_t dest_id, uint32_t source_id, int64_t from_time,
                              uint64_t page_size = 0);

  void require_write_to(int64_t trigger_time, uint32_t source_id, uint32_t dest_id, uint64_t page_size = 0);

  void require_write_to_band(int64_t trigger_time, uint32_t source_id, const yijinjing::data::location_ptr &location,
                             uint64_t page_size = 0) const;

  virtual void react() = 0;

  virtual void on_active() = 0;

  virtual void on_frame() = 0;

  void cleanup_reader_disjoin();

  virtual bool drain(const rx::subscriber<event_ptr> &sb);

  void deal_notice(bool bypass, bool notify, const rx::subscriber<event_ptr> &sb);

private:
  rx::composite_subscription cs_;
  mutable rocksdb::DB *master_db_ = {};
  mutable rocksdb::DB *app_db_ = {};
  mutable std::mutex master_db_mtx_ = {};
  mutable std::mutex app_db_mtx_ = {};
  inline static std::string LOCATION_KEYS = "location_uid64";

  std::unordered_map<uint64_t, longfist::types::Band> bands_ = {};
  std::unordered_map<uint64_t, longfist::types::Channel> channels_ = {};
  std::unordered_map<uint32_t, yijinjing::data::location_ptr> locations_ = {};
  std::unordered_map<uint64_t, yijinjing::data::location_ptr> location64s_ = {};
  std::unordered_map<uint32_t, longfist::types::Register> registry_ = {};
  std::set<uint32_t> disjoin_uids_ = {};
  std::set<std::pair<uint32_t, uint32_t>> disjoin_channels_ = {};
  std::map<yijinjing::data::location_ptr, uint32_t> disjoin_location_channels_;

  volatile bool continual_ = true;

  void produce(const rx::subscriber<event_ptr> &sb);

  template <typename T>
  std::enable_if_t<T::reflect> do_require_read_from(yijinjing::journal::writer_ptr &&writer, int64_t trigger_time,
                                                    uint32_t dest_id, uint32_t source_id, int64_t from_time,
                                                    uint64_t page_size = 0) {
    if (check_location_exists(source_id, dest_id)) {
      T &msg = writer->template open_data<T>(trigger_time);
      msg.source_id = source_id;
      msg.from_time = from_time;
      msg.page_size = page_size;
      writer->close_data();
    }
  }

  static void delegate_produce(hero *instance, const rx::subscriber<event_ptr> &subscriber);

  static void clear_rocksdb(rocksdb::DB **db);
};
} // namespace kungfu::yijinjing::practice
#endif // KUNGFU_HERO_H