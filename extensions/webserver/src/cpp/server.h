#pragma once

#include "kungfu/longfist/types.h"
#include "kungfu/wingchun/book/bookkeeper.h"
#include "kungfu/wingchun/broker/client.h"
#include "kungfu/yijinjing/journal/journal.h"
#include "kungfu/yijinjing/practice/apprentice.h"
#include "nlohmann/json.hpp"
#include "pack_types.h"
#include "threadpoll.h"
#include <atomic>
#include <mutex>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/tls/tls.h>
#include <nng/supplemental/util/platform.h>
#include <nng/transport/tls/tls.h>
#include <string>
#include <thread>
#include <unordered_map>

// const int max_num = 9000;
// static std::unordered_map<int32_t,uint64_t> id_orderinput_map(max_num);
// static std::unordered_map<uint64_t,uint64_t> orderinput_order_map(max_num);

// static std::unordered_map<uint64_t,int32_t> orderinput_id_map(max_num);

// static std::unordered_map<int32_t,uint64_t> id_time_map(max_num);
// static std::unordered_map<uint64_t,uint64_t> orderinput_time_map(max_num);
// static std::unordered_map<uint64_t,uint64_t> order_time_map(max_num);

// static std::unordered_map<uint64_t,uint64_t> before_order_time_map(max_num);
// static std::unordered_map<uint64_t,uint64_t> order_insert_time_map(max_num);
// static std::unordered_map<uint64_t,uint64_t> order_update_time_map(max_num);

namespace kungfu::wingchun::broker {

class TestClient : public AutoClient {
public:
  explicit TestClient(yijinjing::practice::apprentice &app);
  void connect(const event_ptr &event, const longfist::types::Register &register_data) override;
};
} // namespace kungfu::wingchun::broker

namespace kungfu::service {

struct StreamRequestHash {
  std::size_t operator()(const std::pair<uint64_t, uint64_t> &p) const {
    return std::hash<uint64_t>()(p.first) ^ std::hash<uint64_t>()(p.second);
  }
};

struct ServerConfig {
  std::string address;
  std::vector<std::string> paths;
  int thread_num;
};

class server : public kungfu::yijinjing::practice::apprentice {
public:
  explicit server(yijinjing::data::locator_ptr locator, const std::string &group, const std::string &name,
                  longfist::enums::mode m, bool low_latency, const std::string &arguments);
  ~server() override;
  void on_exit() override;
  kungfu::yijinjing::data::location_ptr get_remote_location(uint64_t id);
  bool has_remote_location(uint64_t id);

  // just for test
  template <typename T1, typename T2> void test_fun_orderinput(const void *ptr, uint64_t stream_id) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
    io_network_->get_stream_manager()->publish(stream_id, (char *)(&data_send), sizeof(T2));
    return;
  };

  template <typename T1, typename T2> void test_fun_order(const void *ptr, uint64_t stream_id) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
    io_network_->get_stream_manager()->publish(stream_id, (char *)(&data_send), sizeof(T2));
    return;
  };

  template <typename T1, typename T2> void test_fun_trade(const void *ptr, uint64_t stream_id) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_order_id = kungfu::yijinjing::time::now_in_nano();
    io_network_->get_stream_manager()->publish(stream_id, (char *)(&data_send), sizeof(T2));
    return;
  };
  // test end

  template <typename T1, typename T2> void test_fun(void *ptr, uint64_t stream_id) {
    auto data = static_cast<T1 *>(ptr);
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    io_network_->get_stream_manager()->publish(stream_id, (char *)(&data_send), sizeof(T2));
    return;
  };

protected:
  void on_start() override;
  bool drain(const rx::subscriber<event_ptr> &sb) override;
  void deal_msg(const rx::subscriber<event_ptr> &sb);

private:
  ServerConfig read_config(std::string filename) const;

private:
  kungfu::wingchun::broker::TestClient broker_client_;
  kungfu::yijinjing::io_device_network_ptr io_network_;
  std::mutex asm_mutex_;
  ThreadPool *threadpool_;
  std::unordered_map<uint64_t, kungfu::yijinjing::journal::reader_ptr> stream_reader_map_;
  std::unordered_map<uint64_t, std::shared_ptr<std::thread>> stream_thread_map_;
  std::unordered_map<uint64_t, std::future<void>> stream_task_map_;
  std::unordered_map<uint64_t, kungfu::yijinjing::journal::writer_ptr> stream_writer_map_;
  std::unordered_map<std::pair<uint64_t, uint64_t>, std::uint64_t, StreamRequestHash> request_order_map_ = {};
  std::unordered_map<uint64_t, uint64_t> stream_limit_map_;
  /*
  std::map<int32_t, std::function<void(void *, uint64_t)>> map_event_back = {
      {longfist::types::OrderInput::tag,
       [this](void *ptr, uint64_t stream_id) {
         this->test_fun<longfist::types::OrderInput,
                        CICC::types::PackOrderInput>(ptr, stream_id);
       }},
      {longfist::types::Order::tag,
       [this](void *ptr, uint64_t stream_id) {
         this->test_fun<longfist::types::Order, CICC::types::PackOrder>(
             ptr, stream_id);
       }},
      {longfist::types::Trade::tag,
       [this](void *ptr, uint64_t stream_id) {
         this->test_fun<longfist::types::Trade, CICC::types::PackTrade>(
             ptr, stream_id);
       }},
      {longfist::types::OrderActionError::tag,
       [this](void *ptr, uint64_t stream_id) {
         this->test_fun<longfist::types::OrderActionError,
                        CICC::types::PackOrderActionError>(ptr, stream_id);
       }},
  };
  */
  std::map<int32_t, std::function<void(const void *, uint64_t)>> map_event_back = {
      {longfist::types::OrderInput::tag,
       [this](const void *ptr, uint64_t stream_id) {
         this->test_fun_orderinput<longfist::types::OrderInput, CICC::types::PackOrderInput>(ptr, stream_id);
       }},
      {longfist::types::Order::tag,
       [this](const void *ptr, uint64_t stream_id) {
         this->test_fun_order<longfist::types::Order, CICC::types::PackOrder>(ptr, stream_id);
       }},
      {longfist::types::Trade::tag,
       [this](const void *ptr, uint64_t stream_id) {
         this->test_fun_trade<longfist::types::Trade, CICC::types::PackTrade>(ptr, stream_id);
       }},
  };

  void submit_read_read_assemble();
  void thread_read_data(const kungfu::yijinjing::journal::reader_ptr &reader, uint64_t stream_id);
  void thread_send_data(const kungfu::yijinjing::data::location_ptr &location,
                        uint64_t stream_id);
  void write_data(uint32_t msg_type, const char *msg, uint64_t stream_id);
  bool custom_OnInitEvent(const char *ptr, uint64_t stream_id);
  bool custom_OnNewOrder(const char *ptr, uint64_t stream_id);
  bool custom_OnCancelOrder(const char *ptr, uint64_t stream_id);
  bool custom_OnQryAlgoParentOrder(const char *ptr);
};

} // namespace kungfu::service
