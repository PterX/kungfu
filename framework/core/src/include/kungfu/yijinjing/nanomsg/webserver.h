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

  nng_type **operator&() { return &obj; }

  operator nng_type *() const { return obj; }

  nng_type *operator->() { return obj; }
};

FORWARD_DECLARE_CLASS_PTR(stream_manage)

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
  stream(nng_stream *s, uint64_t stream_id, uint64_t aio_nums = 100);

  virtual ~stream();

  void start_recv();

  void stream_recv_cb();

  int stream_send(const std::string &data);

  int stream_send(const char *data, int len);

  uint64_t get_stream_id() const;

  uint64_t get_opposite_stream_id();

  const yijinjing::data::location_ptr &get_location() const;

private:
  // nng_smart_ptr<nng_aio> aio_send_{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_recv_{nng_aio_free};
  nng_stream *s_;
  std::vector<uint8_t> rec_buffer_;
  uint64_t stream_id_;
  void cancel();
  std::mutex mtx_;
  yijinjing::data::location_ptr location_ = nullptr;
  journal::writer_ptr writer_ = nullptr;
  journal::frame_ptr current_frame_ = nullptr;
  std::vector<nng_smart_ptr<nng_aio>> aio_send_;
  uint64_t aio_nums_;
  uint64_t cur_index_;
  void close_data();
};
DECLARE_PTR(stream)

class webserver {
public:
  stream_manage_ptr stream_manager_;
  // std::map<int, std::shared_ptr<stream>> streams_;
  webserver(stream_manage_ptr stream_manager, const nng_url *base_url, std::string path, bool is_text_mode,
            size_t max_num_connections);

  virtual ~webserver();

  void start_listening();

  void stop_listening();

  void start_accept();

  void accept_cb();

private:
  nng_stream_listener *listener{nullptr};
  nng_smart_ptr<nng_aio> aio_accept{nng_aio_free};
  const nng_url *base_url_;
  std::string path_;
  const bool is_text_mode_;
  const size_t max_num_connections_;
  size_t num_connected_;
};
FORWARD_DECLARE_CLASS_PTR(webserver)

// a http http_server
class http_server {
public:
  std::map<int, std::shared_ptr<webserver>> websockets_;
  explicit http_server(const std::string &address);
  ~http_server();
  void add_websocket(const stream_manage_ptr &stream_manager, const std::string &path, bool is_text_mode,
                     size_t max_num_connections = 0);
  void remove_websocket(int id);
  void start();
  int port();

private:
  nng_smart_ptr<nng_http_server> server_{nng_http_server_release};
  bool started_{false};
  nng_smart_ptr<nng_url> url_{nng_url_free};
};
FORWARD_DECLARE_CLASS_PTR(http_server)

/*
class webclient {
public:
  webclient(const std::string &address, std::function<void(webclient &, const std::string &)> message,
         std::function<void(webclient &)> open, std::function<void(webclient &, const std::string &)> error,
         std::function<void(webclient &)> close, const bool is_text_mode);
  ~webclient();
  void send(const std::string &data);
  void recv_cb();
  void start_recv();

private:
  nng_smart_ptr<nng_stream_dialer> dialer{nng_stream_dialer_free};
  nng_smart_ptr<nng_aio> aio_dialer{nng_aio_free};
  std::function<void(webclient &, const std::string &)> on_message;
  std::function<void(webclient &)> on_open;
  std::function<void(webclient &, const std::string &)> on_error;
  std::function<void(webclient &)> on_close;
};
*/

class webclient {
public:
  webclient(stream_manage_ptr stream_manager, const std::string &address,
            std::function<void(webclient &, const std::string &)> message = nullptr,
            std::function<void(webclient &)> open = nullptr,
            std::function<void(webclient &, const std::string &)> error = nullptr,
            std::function<void(webclient &)> close = nullptr, bool is_text_mode = true);

  virtual ~webclient();

  uint64_t get_stream_id();

private:
  stream_manage_ptr stream_manager_;
  stream_ptr stream_;
  nng_smart_ptr<nng_stream_dialer> dialer{nng_stream_dialer_free};
  nng_smart_ptr<nng_aio> aio_dialer{nng_aio_free};
  std::function<void(webclient &, const std::string &)> on_message;
  std::function<void(webclient &)> on_open;
  std::function<void(webclient &, const std::string &)> on_error;
  std::function<void(webclient &)> on_close;
};
FORWARD_DECLARE_CLASS_PTR(webclient)

class stream_manage {
public:
  explicit stream_manage();

  virtual ~stream_manage() = default;

  int publish(uint64_t stream_id, const std::string &msg);

  int publish(uint64_t stream_id, const char *data, int len);

  std::vector<std::string> get_notice(uint64_t stream_id);

  void clear_notice(uint64_t stream_id);

  stream_ptr get_stream_by_id(uint64_t stream_id);

  std::unordered_map<uint64_t, stream_ptr> &get_all_streams();

  void add_stream(nng_stream *s);

  void add_stream(const stream_ptr &s);

  journal::reader_ptr &get_reader();

  uint64_t get_stream_id(uint32_t location_uid);

private:
  std::unordered_map<uint64_t, stream_ptr> streams_;
  journal::reader_ptr reader_ = nullptr;
  std::map<uint32_t, uint64_t> location_to_stream_id_;
};

/*
std::shared_ptr<http_server> create_server(const std::string url);
std::shared_ptr<webclient> connect_server(const std::string &uri, const bool is_text_mode = true,
                                          std::function<void(webclient &, const std::string &)> on_message = nullptr,
                                          std::function<void(webclient &)> on_open = nullptr,
                                          std::function<void(webclient &, const std::string &)> on_error = nullptr,
                                          std::function<void(webclient &)> on_close = nullptr);
*/
} // namespace kungfu::yijinjing::webserver

#endif // KUNGFU_WEBSERVER_H
