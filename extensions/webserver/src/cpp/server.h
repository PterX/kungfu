#pragma once

#include "kungfu/longfist/types.h"
#include "kungfu/wingchun/book/bookkeeper.h"
#include "kungfu/wingchun/broker/client.h"
#include "kungfu/yijinjing/journal/journal.h"
#include "kungfu/yijinjing/practice/apprentice.h"
#include "nlohmann/json.hpp"
#include <atomic>
#include <mutex>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/tls/tls.h>
#include <nng/supplemental/util/platform.h>
#include <nng/transport/tls/tls.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <kungfu/yijinjing/nanomsg/webserver.h>

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


class ThreadWorker {
public:
  ThreadWorker(yijinjing::webserver::stream_ptr stream) : stream_(stream), ready_(false), live_(true) {}

  void wait() {
    std::unique_lock<std::mutex> lock(mtx);
    // 等待主线程的通知
    cv.wait(lock, [this] { return ready_; });
    ready_ = false;
  }

  void notify() {
    std::lock_guard<std::mutex> lock(mtx);
    ready_ = true;
    cv.notify_all();
  }

  bool is_live() const { return live_; }

  // 通知线程退出
  void stop() {
    live_ = false;
    notify();
  }

  const yijinjing::webserver::stream_ptr &get_stream() { return stream_; }

private:
  std::mutex mtx;
  std::condition_variable cv;
  bool ready_;
  bool live_;
  yijinjing::webserver::stream_ptr stream_;
};
DECLARE_PTR(ThreadWorker)

class server : public kungfu::yijinjing::practice::apprentice {
public:
  explicit server(yijinjing::data::locator_ptr locator, const std::string &group, const std::string &name,
                  longfist::enums::mode m, bool low_latency, const std::string &arguments);
  ~server() override;
  void on_exit() override;
  /*
  // just for test
  template <typename T1, typename T2> void test_fun_orderinput(const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
    //web_agent_->publish((char *)(&data_send), sizeof(T2), stream_id);
    stream->stream_send((char *)(&data_send), sizeof(T2));    
    return;
  };

  template <typename T1, typename T2> void test_fun_order(const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
    //web_agent_->publish((char *)(&data_send), sizeof(T2), stream_id);
    stream->stream_send((char *)(&data_send), sizeof(T2));
    return;
  };

  template <typename T1, typename T2> void test_fun_trade(const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
    auto data = static_cast<T1 *>(const_cast<void *>(ptr));
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    data_send.data.parent_order_id = kungfu::yijinjing::time::now_in_nano();
    //web_agent_->publish((char *)(&data_send), sizeof(T2), stream_id);
    stream->stream_send((char *)(&data_send), sizeof(T2));
    return;
  };
  // test end

  template <typename T1, typename T2> void test_fun(void *ptr, uint64_t stream_id) {
    auto data = static_cast<T1 *>(ptr);
    T2 data_send;
    memcpy(&data_send.data, data, sizeof(T1));
    web_agent_->publish((char *)(&data_send), sizeof(T2), stream_id);
    return;
  };
  */
protected:
  void on_start() override;
  bool drain(const rx::subscriber<event_ptr> &sb) override;
  void deal_msg(const rx::subscriber<event_ptr> &sb);

private:
  ServerConfig read_config(std::string filename) const;

private:
  kungfu::wingchun::broker::TestClient broker_client_;
  kungfu::yijinjing::webserver::web_agent_ptr web_agent_;
  std::mutex asm_mutex_;
  // ThreadPool *threadpool_;
  // std::unordered_map<uint64_t, kungfu::yijinjing::journal::reader_ptr> stream_reader_map_;
  std::unordered_map<uint64_t, std::shared_ptr<std::thread>> stream_thread_map_;
  // std::unordered_map<uint64_t, std::future<void>> stream_task_map_;
  std::unordered_map<uint64_t, kungfu::yijinjing::journal::writer_ptr> stream_writer_map_;
  std::unordered_map<std::pair<uint64_t, uint64_t>, std::uint64_t, StreamRequestHash> request_order_map_ = {};
  std::unordered_map<uint64_t, uint64_t> stream_limit_map_;
  //std::condition_variable cv_;
  //std::unordered_map<uint64_t, std::condition_variable*> stream_cvs_;
  //std::mutex cv_mtx_;

  std::map<uint64_t, ThreadWorker_ptr> stream_workers_ = {};

 /*
  std::map<int32_t, std::function<void(const void *, kungfu::yijinjing::webserver::stream_ptr)>> map_event_back = {
      {longfist::types::OrderInput::tag,
       [this](const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
         this->test_fun_orderinput<longfist::types::OrderInput, CICC::types::PackOrderInput>(ptr, stream);
       }},
      {longfist::types::Order::tag,
       [this](const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
         this->test_fun_order<longfist::types::Order, CICC::types::PackOrder>(ptr, stream);
       }},
      {longfist::types::Trade::tag,
       [this](const void *ptr, kungfu::yijinjing::webserver::stream_ptr stream) {
         this->test_fun_trade<longfist::types::Trade, CICC::types::PackTrade>(ptr, stream);
       }},
  };
*/
  // void submit_read_read_assemble();
  // void thread_read_data(const kungfu::yijinjing::journal::reader_ptr &reader, uint64_t stream_id);
  // void thread_read_data(const kungfu::yijinjing::data::location_ptr &location, ThreadWorker_ptr worker);
  // void thread_send_data(const kungfu::yijinjing::data::location_ptr &location, ThreadWorker_ptr worker);
  // void write_data(uint32_t msg_type, const char *msg, uint64_t stream_id, uint64_t gen_time);
  // bool custom_OnInitEvent(const char *ptr, uint64_t stream_id);
  // bool custom_OnNewOrder(const char *ptr, uint64_t stream_id,uint64_t gen_time);
  // bool custom_OnCancelOrder(const char *ptr, uint64_t stream_id);
  // bool custom_OnQryAlgoParentOrder(const char *ptr);
};

} // namespace kungfu::service
