#include <kungfu/common.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>

namespace kungfu::yijinjing::webserver {
stream::stream(nng_stream *s) : s_(s), rec_buffer_(32768) {
  SPDLOG_DEBUG("stream");

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_recv_,
           [](void *arg) {
             stream *pThis = (stream *)arg;
             pThis->stream_recv_cb();
           },
           this)) != 0) {
    fatal("nng_aio_alloc read", rv);
  }
  if ((rv = nng_aio_alloc(&aio_send_, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc write", rv);
  }
  SPDLOG_DEBUG("before start_recv");
  start_recv();
  SPDLOG_DEBUG("after start_recv");
}
stream::~stream() {
  SPDLOG_DEBUG("~stream");

  cancel();
  nng_stream_free(s_);
  nng_aio_free(aio_recv_);
  nng_aio_free(aio_send_);
}
void stream::start_recv() {
  SPDLOG_DEBUG("start start_recv");
  nng_iov iov = {rec_buffer_.data(), rec_buffer_.size()};
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(s_, aio_recv_);
  SPDLOG_DEBUG("end start_recv");
}
void stream::stream_recv_cb() {
  SPDLOG_DEBUG("stream_recv_cb");
  int rv = nng_aio_result(aio_recv_);
  auto len = nng_aio_count(aio_recv_);
  switch (rv) {
  case 0: {
    // std::string data((char *)rec_buffer_.data(), len);
    {
      std::lock_guard<std::mutex> lock(mtx_);
      data_received_.emplace_back((char *)rec_buffer_.data(), len);
    }
    start_recv();
    break;
  }
  case NNG_ECLOSED: {
    SPDLOG_DEBUG("NNG_ECLOSED");
    // disposer_(this);
    break;
  }
  default:
    SPDLOG_DEBUG("default");
    break;
  }
}
void stream::stream_send(const std::string &data) {
  nng_iov iov;
  iov.iov_buf = (void *)data.data();
  iov.iov_len = data.size();
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(s_, aio_send_);
  // nng_aio_wait(aio_send_);
  rv = nng_aio_result(aio_send_);
  if (rv != 0) {
    fatal("nng_aio_result", rv);
  }
}
void stream::cancel() {
  nng_aio_cancel(aio_recv_);
  nng_aio_wait(aio_recv_);
  nng_aio_cancel(aio_send_);
  nng_aio_wait(aio_send_);
}

websocket::websocket(const nng_url *base_url, const std::string &path, const bool text_mode,
                     const size_t max_num_connections)
    : base_url_(base_url), path_(path), text_mode_(text_mode), max_num_connections_(max_num_connections) {
  SPDLOG_DEBUG("websocket");

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_accept, [](void *arg) { ((websocket *)arg)->accept_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  start_listening();
}
websocket::~websocket() {
  SPDLOG_DEBUG("~websocket");

  streams.clear();
  nng_aio_cancel(aio_accept);
  stop_listening();
}
void websocket::start_listening() {
  nng_url url = *base_url_;
  const bool secure = (strcmp(base_url_->u_scheme, "https") == 0);
  url.u_path = (char *)path_.c_str();
  url.u_scheme = (char *)(secure ? "wss" : "ws");
  int rv = nng_stream_listener_alloc_url(&listener, &url);
  if (rv != 0) {
    fatal("nng_ststener_alloc_url", rv);
  }
  nng_stream_listener_set_bool(listener, NNG_OPT_TCP_NODELAY, true);
  nng_stream_listener_set_bool(listener, NNG_OPT_TCP_KEEPALIVE, true);
  nng_stream_listener_set_size(listener, NNG_OPT_WS_SENDMAXFRAME, 1000000);
  if (text_mode_) {
    nng_stream_listener_set_bool(listener, NNG_OPT_WS_SEND_TEXT, true);
    nng_stream_listener_set_bool(listener, NNG_OPT_WS_RECV_TEXT, true);
  }

  if ((rv = nng_stream_listener_listen(listener)) != 0) {
    fatal("nng_stream_listener_alloc_url", rv);
  }

  start_accept();
}
void websocket::stop_listening() {
  if (listener != nullptr) {
    nng_stream_listener_close(listener);
    nng_stream_listener_free(listener);
    listener = nullptr;
  }
}
void websocket::start_accept() { nng_stream_listener_accept(listener, aio_accept); }
void websocket::accept_cb() {
  int rv = nng_aio_result(aio_accept);
  if (rv != 0) {
    return;
  }

  nng_stream *s = (nng_stream *)nng_aio_get_output(aio_accept, 0);

  try {
    if (max_num_connections_ > 0 && (streams.size() + 1) >= max_num_connections_) {
      stop_listening();
    } else {
      start_accept();
    }
    auto id = streams.empty() ? 1 : streams.rbegin()->first + 1;
    auto impl = std::unique_ptr<stream>(new stream(s));
    streams.insert(std::make_pair(id, std::move(impl)));
  } catch (std::exception &) {
  }
}

server::server(const std::string address) {
  SPDLOG_DEBUG("server");
  int rv;
  if ((rv = nng_url_parse(&url_, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_http_server_hold(&server_, url_)) != 0) {
    fatal("nng_http_server_hold", rv);
  }
}

server::~server() {
  SPDLOG_DEBUG("~server");
  // If any of these assert, some RouteHolder object is still active
  assert(websockets_.empty());

  if (server_ != nullptr) {
    nng_http_server_stop(server_);
    nng_http_server_release(server_);
  }
}

void server::add_websocket(const std::string &path, bool is_text_mode, const size_t max_num_connections) {
  auto socket = std::unique_ptr<websocket>(new websocket(url_, path, is_text_mode, max_num_connections));
  const auto id = websockets_.empty() ? 1 : websockets_.rbegin()->first + 1;
  websockets_.emplace(std::make_pair(id, std::move(socket)));
  return;
}

void server::remove_websocket(int id) {
  // std::cout << "remove_websocket()" << std::endl;
  // std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
  auto it = websockets_.find(id);
  if (it != websockets_.end()) {
    websockets_.erase(it);
  }
}

void server::start() {
  int rv;
  if (started_) {
    return;
  }
  if ((rv = nng_http_server_start(server_)) != 0) {
    fatal("nng_http_server_start", rv);
  }
  started_ = true;
  SPDLOG_INFO("server started, listening on port {}", port());
}

int server::port() {
  if (!started_) {
    throw std::runtime_error("server not started");
  }
  nng_sockaddr addr;
  int rv;
  if ((rv = nng_http_server_get_addr(server_, &addr)) != 0) {
    fatal("nng_http_server_get_port", rv);
  }
  return ntohs(addr.s_in.sa_port);
}

client::client(const std::string &address, std::function<void(client &, const std::string &)> message,
               std::function<void(client &)> open, std::function<void(client &, const std::string &)> error,
               std::function<void(client &)> close, const bool text_mode)
    : on_message(message), on_open(open), on_error(error), on_close(close), buffer(32768) {
  int rv;
  nng_smart_ptr<nng_url> url{nng_url_free};
  if ((rv = nng_url_parse(&url, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_stream_dialer_alloc_url(&dialer, url)) != 0) {
    fatal("nng_stream_dialer_alloc", rv);
  }
  if ((rv = nng_aio_alloc(&aio_dialer, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  if ((rv = nng_aio_alloc(
           &aio_read, [](void *arg) { ((client *)arg)->recv_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  if ((rv = nng_aio_alloc(&aio_write, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc", rv);
  }

  if (text_mode) {
    nng_stream_dialer_set_bool(dialer, NNG_OPT_WS_RECV_TEXT, true);
    nng_stream_dialer_set_bool(dialer, NNG_OPT_WS_SEND_TEXT, true);
  }

  nng_stream_dialer_dial(dialer, aio_dialer);
  nng_aio_wait(aio_dialer);
  rv = nng_aio_result(aio_dialer);
  if (rv != 0) {
    fatal("dial", rv);
  }
  stream = (nng_stream *)nng_aio_get_output(aio_dialer, 0);
  if (on_open) {
    on_open(*this);
  }
  start_recv();
}

client::~client() {
  nng_aio_cancel(aio_dialer);
  nng_aio_cancel(aio_read);
  nng_aio_cancel(aio_write);
  nng_aio_wait(aio_read);
  nng_aio_wait(aio_write);
  nng_stream_dialer_close(dialer);
}

void client::send(const std::string &data) {
  nng_iov iov{(void *)data.data(), data.size()};
  nng_aio_set_iov(aio_write, 1, &iov);
  nng_stream_send(stream, aio_write);
  nng_aio_wait(aio_write);
  int rv = nng_aio_result(aio_write);
  auto send_time = std::chrono::high_resolution_clock::now();
  // std::cout << "send data:" << data << std::endl;
  if (rv != 0) {
    fatal("nng_aio_result", rv);
  }
}

void client::recv_cb() {
  int rv = nng_aio_result(aio_read);
  if (rv != 0) {
    if (rv == NNG_ECLOSED || rv == NNG_ECANCELED) {
      if (on_close) {
        on_close(*this);
      }
    } else {
      if (on_error) {
        on_error(*this, nng_strerror(rv));
      }
    }
    return;
  }
  auto len = nng_aio_count(aio_read);
  std::string data((const char *)buffer.data(), len);
  if (on_message) {
    on_message(*this, data);
    auto recv_time = std::chrono::high_resolution_clock::now();
  }
  start_recv();
}

void client::start_recv() {
  nng_iov iov{buffer.data(), buffer.size()};
  nng_aio_set_iov(aio_read, 1, &iov);
  nng_stream_recv(stream, aio_read);
}

std::shared_ptr<server> create_server(const std::string url) { return std::make_shared<server>(url); }
std::shared_ptr<client> connect_server(const std::string &uri,
                                       std::function<void(client &, const std::string &)> on_message,
                                       std::function<void(client &)> on_open,
                                       std::function<void(client &, const std::string &)> on_error,
                                       std::function<void(client &)> on_close, const bool is_text_mode) {
  return std::make_shared<client>(uri, on_message, on_open, on_error, on_close, is_text_mode);
}

} // namespace kungfu::yijinjing::webserver