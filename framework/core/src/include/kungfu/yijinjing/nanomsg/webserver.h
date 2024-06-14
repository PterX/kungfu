#ifndef KUNGFU_WEBSERVER_H
#define KUNGFU_WEBSERVER_H

#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/journal.h>
#include <nng/nng.h>
#include <nng/supplemental/http/http.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <shared_mutex>

#include <sstream>
#include <stdexcept>

#include <cassert>

#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace kungfu::yijinjing::webserver {

uint64_t roundup_pow_of_two(const uint64_t x);

class webserver_error : public std::runtime_error {
public:
  explicit webserver_error(const std::string &message) : runtime_error(message) { SPDLOG_CRITICAL(message); }
};

static void fatal(const char *what, int rv) {
  std::stringstream ss;
  ss << what << ": " << nng_strerror(rv);
  throw webserver_error(ss.str());
}

template <class nng_type> class nng_smart_ptr {
  using nng_free_function = std::function<void(nng_type *)>;
  nng_type *obj{nullptr};
  nng_free_function fn_free;
  void release() {
    if (obj != nullptr) {
      fn_free(obj);
      obj = nullptr;
    }
  }

public:
  explicit nng_smart_ptr(nng_free_function fn) : fn_free(fn) {}

  explicit nng_smart_ptr(nng_type *nng_pointer, nng_free_function fn) : obj(nng_pointer), fn_free(fn) {}

  ~nng_smart_ptr() { release(); }
  nng_smart_ptr &operator=(nng_type *new_obj) {
    release();
    obj = new_obj;
    return *this;
  }


  void reset(nng_type *new_obj = nullptr) {
    release();
    obj = new_obj;
  }

  nng_type **operator&() { return &obj; }

  operator nng_type *() const { return obj; }

  nng_type *operator->() { return obj; }
};

// remote_ip:remote_port:local_port
static uint64_t generate_stream_id(nng_stream *s) {
  nng_sockaddr local_address, remote_address;
  nng_stream_get_addr(s, NNG_OPT_REMADDR, &remote_address);
  nng_stream_get_addr(s, NNG_OPT_LOCADDR, &local_address);
  return (static_cast<uint64_t>(remote_address.s_in.sa_addr) << 32) |
         (static_cast<uint64_t>(remote_address.s_in.sa_port) << 16) | local_address.s_in.sa_port;
}

class stream {
public:
  stream(nng_stream *s, uint64_t stream_id, uint64_t aio_nums = 1024);

  virtual ~stream();

  int stream_send(const std::string &data);

  int stream_send(const char *data, int len);

  uint64_t get_stream_id() const;

  uint64_t get_opposite_stream_id();

  const yijinjing::data::location_ptr &get_location() const;

  void cancel();

private:
  void close_data();
  
  void start_recv();

  void stream_recv_cb();

  void stream_send_cb();

private:
  nng_smart_ptr<nng_aio> aio_send_{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_recv_{nng_aio_free};
  nng_smart_ptr<nng_stream> stream_;
  uint64_t stream_id_;
  yijinjing::data::location_ptr location_ = nullptr;
  journal::writer_ptr writer_ = nullptr;
  journal::frame_ptr current_frame_ = nullptr;
  //std::vector<nng_smart_ptr<nng_aio>> aio_send_;
  uint64_t aio_nums_;
  uint64_t cur_index_;
};
DECLARE_PTR(stream)


class stream_manage {
public:
  explicit stream_manage();

  virtual ~stream_manage() = default;

  int publish(uint64_t stream_id, const std::string &msg);

  int publish(uint64_t stream_id, const char *data, int len);

  stream_ptr get_stream_by_id(uint64_t stream_id);

  void add_stream(nng_stream *s);

  void add_stream(const stream_ptr &s);

  journal::reader_ptr &get_reader();

  uint64_t get_stream_id(uint32_t location_uid);

private:
  std::unordered_map<uint64_t, stream_ptr> streams_;
  std::shared_mutex mtx_;
  journal::reader_ptr reader_ = nullptr;
  std::map<uint32_t, uint64_t> location_to_stream_id_;
};
DECLARE_PTR(stream_manage)

class web_agent{
public:
  explicit web_agent(stream_manage_ptr stream_manager = nullptr):stream_manager_(stream_manager ? stream_manager : std::make_shared<kungfu::yijinjing::webserver::stream_manage>()) {};
  
  virtual void start() = 0;

  virtual void stop() = 0;

  virtual void publish(const char *data, int len, uint64_t stream_id) = 0;

  virtual stream_manage_ptr get_stream_manager(){return stream_manager_;}
  
  virtual stream_ptr get_stream_by_id(uint64_t stream_id){return stream_manager_->get_stream_by_id(stream_id);}
private:
  stream_manage_ptr stream_manager_;
};
DECLARE_PTR(web_agent)

class websocket_server: public web_agent{
public:
  explicit websocket_server(stream_manage_ptr stream_manager, const nng_url *base_url, std::string path, bool is_text_mode,
            bool tcp_no_delay, size_t max_num_connections);

  // Do not support start a websocket service just by a websocket_server object,
  // should start a http_server then add websocket_server
  // explicit websocket_server(stream_manage_ptr stream_manager, const char *url, bool is_text_mode,
  //           bool tcp_no_delay, size_t max_num_connections);

  virtual ~websocket_server();

  void start() override;

  void stop() override;

  void start_accept();

  void accept_cb();

  void publish(const char *data, int len, uint64_t stream_id = 0) override;
private:
  nng_smart_ptr<nng_stream_listener> listener_{nng_stream_listener_free};
  //nng_stream_listener *listener_{nullptr};
  nng_smart_ptr<nng_aio> aio_accept_{nng_aio_free};
  const nng_url *url_;
  const bool is_text_mode_;
  const bool tcp_no_delay_;
  const size_t max_num_connections_;
  size_t num_connected_;
};
FORWARD_DECLARE_CLASS_PTR(websocket_server)

// a http http_server
class http_server: public web_agent{
public:
  explicit http_server(const std::string &address);

  ~http_server();

  void add_websocket(const std::string &path, bool is_text_mode,
                     bool tcp_no_delay = true, size_t max_num_connections = 0);

  void remove_websocket(const std::string& path);

  int port();

  void start() override;

  void stop() override;
  
  void publish(const char *data, int len, uint64_t stream_id = 0) override;
private:
  std::map<std::string, std::shared_ptr<websocket_server>> websockets_;
  nng_smart_ptr<nng_http_server> server_{nng_http_server_release};
  bool started_;
  nng_smart_ptr<nng_url> url_{nng_url_free};
};
FORWARD_DECLARE_CLASS_PTR(http_server)

class websocket_client: public web_agent{
public:
  explicit websocket_client(stream_manage_ptr stream_manager, const std::string &address,
                   bool is_text_mode = true, bool tcp_no_delay = true);

  virtual ~websocket_client();

  uint64_t get_stream_id();

  int send_msg(const char *data, int data_len);
  
  stream_ptr get_stream();

  void start() override;

  void stop() override;

  void publish(const char *data, int len, uint64_t stream_id = 0) override;
private:
  stream_ptr stream_;
  nng_smart_ptr<nng_stream_dialer> dialer_{nng_stream_dialer_free};
  nng_smart_ptr<nng_aio> aio_dialer_{nng_aio_free};
};
FORWARD_DECLARE_CLASS_PTR(websocket_client)

} // namespace kungfu::yijinjing::webserver

#endif // KUNGFU_WEBSERVER_H
