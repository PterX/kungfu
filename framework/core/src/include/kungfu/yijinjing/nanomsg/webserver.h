#ifndef KUNGFU_WEBserver_H
#define KUNGFU_WEBserver_H

#include <kungfu/common.h>

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

template <class nng_type, class nng_free_function = std::function<void(nng_type *)>> class nng_smart_ptr {
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

FORWARD_DECLARE_CLASS_PTR(stream)
FORWARD_DECLARE_CLASS_PTR(websocket)
FORWARD_DECLARE_CLASS_PTR(server)

class stream {
private:
  nng_smart_ptr<nng_aio> aio_send_{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_recv_{nng_aio_free};
  nng_stream *s_;
  std::vector<uint8_t> rec_buffer_;
  std::mutex mtx_;

public:
  std::vector<std::string> data_received_;
  stream(nng_stream *s);
  ~stream();
  void start_recv();
  void stream_recv_cb();
  void stream_send(const std::string &data);

private:
  void cancel();
};

class websocket {
private:
  nng_stream_listener *listener{nullptr};
  nng_smart_ptr<nng_aio> aio_accept{nng_aio_free};
  const nng_url *base_url_;
  std::string path_;
  const bool text_mode_;
  const size_t max_num_connections_;
public:
  std::map<int, std::shared_ptr<stream>> streams_;
  websocket(const nng_url *base_url, const std::string &path, const bool text_mode, const size_t max_num_connections);
  ~websocket();
  void start_listening();
  void stop_listening();
  void start_accept();
  void accept_cb();
};

// a http server
class server {
private:
  nng_smart_ptr<nng_http_server> server_{nng_http_server_release};
  bool started_{false};
  nng_smart_ptr<nng_url> url_{nng_url_free};
public:
  std::map<int, std::shared_ptr<websocket>> websockets_;
  server(const std::string address);
  ~server();
  void add_websocket(const std::string &path, bool is_text_mode, const size_t max_num_connections = 0);
  void remove_websocket(int id);
  void start();
  int port();
};

class client {
public:
  client(const std::string &address, std::function<void(client &, const std::string &)> message,
         std::function<void(client &)> open, std::function<void(client &, const std::string &)> error,
         std::function<void(client &)> close, const bool text_mode);
  ~client();
  void send(const std::string &data);
  void recv_cb();
  void start_recv();

private:
  nng_smart_ptr<nng_stream_dialer> dialer{nng_stream_dialer_free};
  nng_smart_ptr<nng_aio> aio_dialer{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_read{nng_aio_free};
  nng_smart_ptr<nng_aio> aio_write{nng_aio_free};
  nng_smart_ptr<nng_stream> stream{nng_stream_free};
  std::vector<uint8_t> buffer;
  std::function<void(client &, const std::string &)> on_message;
  std::function<void(client &)> on_open; 
  std::function<void(client &, const std::string &)> on_error;
  std::function<void(client &)> on_close;
};

std::shared_ptr<server> create_server(const std::string url);
std::shared_ptr<client> connect_server(const std::string &uri,
                                       std::function<void(client &, const std::string &)> on_message,
                                       std::function<void(client &)> on_open = nullptr,
                                       std::function<void(client &, const std::string &)> on_error = nullptr,
                                       std::function<void(client &)> on_close = nullptr,
                                       const bool is_text_mode = true);

} // namespace kungfu::yijinjing::webserver

#endif // KUNGFU_WEBserver_H
