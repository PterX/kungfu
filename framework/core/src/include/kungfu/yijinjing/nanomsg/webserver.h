#ifndef KUNGFU_WEBserver_H
#define KUNGFU_WEBserver_H

#include <kungfu/common.h>
#include <kungfu/yijinjing/journal/assemble.h>


#include <nng/nng.h>
#include <nng/supplemental/http/http.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include <sstream>
#include <stdexcept>

#include <assert.h>

#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace kungfu::yijinjing::webserver {

static void fatal(const char *what, int rv) {
  std::stringstream ss;
  ss << what << ": " << nng_strerror(rv);
  throw std::runtime_error(ss.str());
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
  nng_smart_ptr(nng_free_function fn) : fn_free(fn) {}
  nng_smart_ptr(nng_type *nng_pointer, nng_free_function fn) : obj(nng_pointer), fn_free(fn) {}
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
};

class stream {
private:
  nng_smart_ptr<nng_aio> aio_send_{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_recv_{nng_aio_free};
  nng_stream *s_;
  std::vector<uint8_t> rec_buffer_;
  uint64_t stream_id_;
  void cancel();
  std::mutex mtx_;
  std::thread asm_read_thread_;
  kungfu::yijinjing::journal::assemble_ptr asm_read_ = nullptr;
  void thread_read_data(kungfu::longfist::types::RequestRemoteData data);
public:
  // the first vector is used for callback function receive data, the second vector is used for cache the data have
  // received when call get_data(), will return the
  std::vector<std::string> data_received_;       // used for receive data
  std::vector<std::string> data_received_cache_; // cache data_received
  stream(nng_stream* s, uint64_t stream_id, uint32_t buffer_size = 32768);
  ~stream();
  void start_recv();
  void stream_recv_cb();
  void stream_send(const std::string &data);
  uint64_t get_stream_id();
};
DECLARE_PTR(stream)


class webserver {
private:
  nng_stream_listener *listener{nullptr};
  nng_smart_ptr<nng_aio> aio_accept{nng_aio_free};
  const nng_url *base_url_;
  std::string path_;
  const bool is_text_mode_;
  const size_t max_num_connections_;
  size_t num_connected_;

public:
  stream_manage_ptr stream_manager_;
  // std::map<int, std::shared_ptr<stream>> streams_;
  webserver(stream_manage_ptr stream_manager,const nng_url *base_url, const std::string &path, const bool is_text_mode,
            const size_t max_num_connections);
  ~webserver();
  void start_listening();
  void stop_listening();
  void start_accept();
  void accept_cb();
};
FORWARD_DECLARE_CLASS_PTR(webserver)

// a http http_server
class http_server {
private:
  nng_smart_ptr<nng_http_server> server_{nng_http_server_release};
  bool started_{false};
  nng_smart_ptr<nng_url> url_{nng_url_free};

public:
  std::map<int, std::shared_ptr<webserver>> websockets_;
  http_server(const std::string address);
  ~http_server();
  void add_websocket(stream_manage_ptr stream_manager, const std::string &path, bool is_text_mode, const size_t max_num_connections = 0);
  void remove_websocket(int id);
  void start();
  int port();
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
  webclient(stream_manage_ptr stream_manager,const std::string &address, std::function<void(webclient &, const std::string &)> message = nullptr,
            std::function<void(webclient &)> open = nullptr, std::function<void(webclient &, const std::string &)> error = nullptr,
            std::function<void(webclient &)> close = nullptr, const bool is_text_mode = true);
  ~webclient();
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
  stream_manage();
  ~stream_manage();
  int publish(uint64_t stream_id, const std::string &msg);
  std::vector<std::string> &get_notice(uint64_t stream_id);
  stream_ptr get_stream_by_id(uint64_t stream_id);
  std::unordered_map<uint64_t, stream_ptr> &get_all_streams();
  void add_stream(nng_stream *s);
  void add_stream(stream_ptr s);
private:
  std::unordered_map<uint64_t, stream_ptr> streams_;
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

#endif // KUNGFU_WEBserver_H
