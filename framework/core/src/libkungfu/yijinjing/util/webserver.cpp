#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>
#include <memory>
#include <thread>
#include <utility>

using namespace kungfu::yijinjing::data;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing::journal;

namespace kungfu::yijinjing::webserver {
constexpr uint64_t PAGE_SIZE = 256;

stream::stream(nng_stream *s, uint64_t stream_id) : s_(s), stream_id_(stream_id) {
  SPDLOG_DEBUG("stream");
  location_ = location::make_shared(mode::LIVE, category::SYSTEM, "webserver", std::to_string(stream_id),
                                    std::make_shared<locator>(mode::LIVE));
  writer_ = std::make_shared<writer>(location_, location::PUBLIC, false, std::make_shared<noop_publisher>(), true,
                                     std::make_shared<bus>(false), PAGE_SIZE);
  reader_ = std::make_shared<reader>(true, true, std::make_shared<bus>(false));
  reader_->join(location_, location::PUBLIC, time::now_in_nano());

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_recv_,
           [](void *arg) {
             auto *pThis = reinterpret_cast<stream *>(arg);
             pThis->stream_recv_cb();
           },
           this)) != 0) {
    fatal("nng_aio_alloc read", rv);
  }
  if ((rv = nng_aio_alloc(&aio_send_, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc write", rv);
  }
  start_recv();
}

stream::~stream() {
  SPDLOG_DEBUG("~stream");

  close_data();
  cancel();
  nng_stream_free(s_);
  nng_aio_free(aio_recv_);
  nng_aio_free(aio_send_);
}

uint64_t stream::get_stream_id() const { return stream_id_; }

uint64_t stream::get_opposite_stream_id() {
  nng_sockaddr local_address, remote_address;
  nng_stream_get_addr(s_, NNG_OPT_REMADDR, &remote_address);
  nng_stream_get_addr(s_, NNG_OPT_LOCADDR, &local_address);
  return (static_cast<uint64_t>(local_address.s_in.sa_addr) << 32) |
         (static_cast<uint64_t>(local_address.s_in.sa_port) << 16) | remote_address.s_in.sa_port;
}

void stream::close_data() {
  if (current_frame_) {
    //    SPDLOG_INFO("close_frame_lock_free");
    writer_->close_frame_lock_free(1024);
    current_frame_.reset();
  }
}

void stream::start_recv() {
  close_data();
  //  SPDLOG_INFO("open_frame_lock_free");
  current_frame_ = writer_->open_frame_lock_free(time::now_in_nano(), 10001000, 1024);
  nng_iov iov{const_cast<void *>(current_frame_->data_address()), current_frame_->data_length()};
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(s_, aio_recv_);
}

void stream::stream_recv_cb() {
  int rv = nng_aio_result(aio_recv_);

  auto len = nng_aio_count(aio_recv_);
  switch (rv) {
  case 0: {
    //    {
    //      std::string data((char *)rec_buffer_.data(), len);
    //      std::lock_guard<std::mutex> lock(mtx_);
    //      data_received_.emplace_back((char *)rec_buffer_.data(), len);
    //    }
    start_recv();
    break;
  }
  case NNG_ECLOSED: {
    SPDLOG_DEBUG("NNG_ECLOSED");
    // 写入特殊内容到current_frame里去, 然后close_data();
    // disposer_(this);
    break;
  }
  default:
    SPDLOG_DEBUG("default:{}", rv);
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
  nng_aio_wait(aio_send_);
  rv = nng_aio_result(aio_send_);
  if (rv != 0) {
    fatal("nng_aio_result", rv);
  }
}

int stream::stream_send(const char *data, const int len) {
  nng_iov iov;
  iov.iov_buf = (void *)data;
  iov.iov_len = len;
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(s_, aio_send_);
  nng_aio_wait(aio_send_);
  rv = nng_aio_result(aio_send_);
  if (rv != 0) {
    fatal("nng_aio_result", rv);
  }
  return rv;
}

std::vector<std::string> stream::get_and_clear_data() {
  //  std::lock_guard<std::mutex> lock(mtx_);
  //  std::vector<std::string> result = data_received_; // 返回数据的副本
  //  data_received_.clear();
  std::vector<std::string> result{};
  int count = 0;
  auto &jour = get_journal();
  while (jour.current_frame()->has_data() and count < 100) {
    result.push_back(jour.current_frame()->data_as_string());
    jour.next();
    ++count;
    //    SPDLOG_INFO("data_available, count: {}", count);
  }
  return result;
}

journal::reader_ptr &stream::get_reader() { return reader_; }

void stream::cancel() {
  nng_aio_cancel(aio_recv_);
  nng_aio_wait(aio_recv_);
  nng_aio_cancel(aio_send_);
  nng_aio_wait(aio_send_);
}

journal::journal &stream::get_journal() { return reader_->get_journal_ref(location_, location::PUBLIC); }

webserver::webserver(stream_manage_ptr stream_manager, const nng_url *base_url, std::string path,
                     const bool is_text_mode, const size_t max_num_connections)
    : stream_manager_(std::move(stream_manager)), base_url_(base_url), path_(std::move(path)),
      is_text_mode_(is_text_mode), max_num_connections_(max_num_connections), num_connected_(0) {
  SPDLOG_DEBUG("webserver");

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_accept, [](void *arg) { ((webserver *)arg)->accept_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  start_listening();
}

webserver::~webserver() {
  SPDLOG_DEBUG("~webserver");

  // streams_.clear();
  nng_aio_cancel(aio_accept);
  stop_listening();
}

void webserver::start_listening() {
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
  if (is_text_mode_) {
    nng_stream_listener_set_bool(listener, NNG_OPT_WS_SEND_TEXT, true);
    nng_stream_listener_set_bool(listener, NNG_OPT_WS_RECV_TEXT, true);
  }

  if ((rv = nng_stream_listener_listen(listener)) != 0) {
    fatal("nng_stream_listener_alloc_url", rv);
  }

  start_accept();
}

void webserver::stop_listening() {
  if (listener != nullptr) {
    nng_stream_listener_close(listener);
    nng_stream_listener_free(listener);
    listener = nullptr;
  }
}

void webserver::start_accept() { nng_stream_listener_accept(listener, aio_accept); }

void webserver::accept_cb() {
  int rv = nng_aio_result(aio_accept);
  if (rv != 0) {
    return;
  }

  auto *s = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_accept, 0));
  // disable Nagle, send-msg low-latency
  nng_stream_set_bool(s, NNG_OPT_TCP_NODELAY, true);
  //nng_stream_set_ms(s, NNG_OPT_RECVTIMEO, 0);
  //nng_stream_set_ms(s, NNG_OPT_SENDTIMEO, 0);

  try {
    if (max_num_connections_ > 0 && (num_connected_ + 1) >= max_num_connections_) {
      stop_listening();
    } else {
      start_accept();
    }
    stream_manager_->add_stream(s);
    num_connected_++;
  } catch (std::exception &) {
  }
}

http_server::http_server(const std::string &address) {
  SPDLOG_DEBUG("http_server");
  int rv;
  if ((rv = nng_url_parse(&url_, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_http_server_hold(&server_, url_)) != 0) {
    fatal("nng_http_server_hold", rv);
  }
  this->start();
}

http_server::~http_server() {
  SPDLOG_DEBUG("~http_server");
  // If any of these assert, some RouteHolder object is still active
  assert(websockets_.empty());

  if (server_ != nullptr) {
    nng_http_server_stop(server_);
    nng_http_server_release(server_);
  }
}

void http_server::add_websocket(const stream_manage_ptr &stream_manager, const std::string &path, bool is_text_mode,
                                const size_t max_num_connections) {
  auto websocket = std::make_shared<webserver>(stream_manager, url_, path, is_text_mode, max_num_connections);
  const auto id = websockets_.empty() ? 1 : websockets_.rbegin()->first + 1;
  websockets_.emplace(std::make_pair(id, std::move(websocket)));
}

void http_server::remove_websocket(int id) {
  // std::cout << "remove_websocket()" << std::endl;
  // std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
  auto it = websockets_.find(id);
  if (it != websockets_.end()) {
    websockets_.erase(it);
  }
}

void http_server::start() {
  int rv;
  if (started_) {
    return;
  }
  if ((rv = nng_http_server_start(server_)) != 0) {
    fatal("nng_http_server_start", rv);
  }
  started_ = true;
  SPDLOG_INFO("http_server started, listening on port {}", port());
}

int http_server::port() {
  if (!started_) {
    throw webserver_error("http_server not started");
  }
  nng_sockaddr addr;
  int rv;
  if ((rv = nng_http_server_get_addr(server_, &addr)) != 0) {
    fatal("nng_http_server_get_port", rv);
  }
  return ntohs(addr.s_in.sa_port);
}

webclient::webclient(stream_manage_ptr stream_manager, const std::string &address,
                     std::function<void(webclient &, const std::string &)> message,
                     std::function<void(webclient &)> open, std::function<void(webclient &, const std::string &)> error,
                     std::function<void(webclient &)> close, const bool is_text_mode)
    : stream_manager_(std::move(stream_manager)), on_message(std::move(message)), on_open(std::move(open)),
      on_error(std::move(error)), on_close(std::move(close)) {
  SPDLOG_DEBUG("webclient");
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

  if (is_text_mode) {
    nng_stream_dialer_set_bool(dialer, NNG_OPT_WS_RECV_TEXT, true);
    nng_stream_dialer_set_bool(dialer, NNG_OPT_WS_SEND_TEXT, true);
  }

  nng_stream_dialer_dial(dialer, aio_dialer);
  nng_aio_wait(aio_dialer);
  rv = nng_aio_result(aio_dialer);
  if (rv != 0) {
    fatal("dial", rv);
  }
  auto *s = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_dialer, 0));
  // disable Nagle, send-msg low-latency
  nng_stream_set_bool(s, NNG_OPT_TCP_NODELAY, true);
  //nng_stream_set_ms(s, NNG_OPT_RECVTIMEO, 0);
  //nng_stream_set_ms(s, NNG_OPT_SENDTIMEO, 0);

  auto temp_stream = std::make_shared<stream>(s, generate_stream_id(s));
  stream_ = temp_stream;
  stream_manager_->add_stream(temp_stream);
}

webclient::~webclient() { SPDLOG_DEBUG("~webclient"); }

uint64_t webclient::get_stream_id() { return stream_->get_stream_id(); }

int stream_manage::publish(uint64_t stream_id, const std::string &msg) {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  if (!streams_.contains(stream_id)) {
    SPDLOG_ERROR("publish failed:{}", msg);
    return -1;
  }
  streams_.at(stream_id)->stream_send(msg);
  return 0;
}
int stream_manage::publish(uint64_t stream_id, const char *data, const int len) {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  if (!streams_.contains(stream_id)) {
    SPDLOG_DEBUG("publish failed");
    return -1;
  }
  streams_.at(stream_id)->stream_send(data, len);
  // SPDLOG_DEBUG("publish success");
  return 0;
}

std::vector<std::string> stream_manage::get_notice(uint64_t stream_id) {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  return streams_.find(stream_id)->second->get_and_clear_data();
}

void stream_manage::clear_notice(uint64_t stream_id) {
  //  streams_.find(stream_id)->second->data_received_.clear();
}

stream_ptr stream_manage::get_stream_by_id(uint64_t stream_id) {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  return streams_.find(stream_id)->second;
}
std::unordered_map<uint64_t, stream_ptr> &stream_manage::get_all_streams() {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  return streams_;
}

void stream_manage::add_stream(nng_stream *s) {
  SPDLOG_DEBUG("add_stream");
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  auto temp_stream = std::make_shared<stream>(s, generate_stream_id(s));
  streams_.emplace(temp_stream->get_stream_id(), temp_stream);
}

void stream_manage::add_stream(const stream_ptr &s) {
  // std::lock_guard<std::mutex> lock(streams_mtx_);
  SPDLOG_DEBUG("add_stream");
  streams_.emplace(s->get_stream_id(), s);
}

journal::reader_ptr stream_manage::get_reader(uint64_t stream_id) {
  auto iter = streams_.find(stream_id);
  if (iter != streams_.end()) {
    return iter->second->get_reader();
  }
  return nullptr;
}

/*
webclient::webclient(const std::string &address, std::function<void(webclient &, const std::string &)> message,
               std::function<void(webclient &)> open, std::function<void(webclient &, const std::string &)> error,
               std::function<void(webclient &)> close, const bool is_text_mode)
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
           &aio_read, [](void *arg) { ((webclient *)arg)->recv_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  if ((rv = nng_aio_alloc(&aio_write, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc", rv);
  }

  if (is_text_mode) {
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

webclient::~webclient() {
  nng_aio_cancel(aio_dialer);
  nng_aio_cancel(aio_read);
  nng_aio_cancel(aio_write);
  nng_aio_wait(aio_read);
  nng_aio_wait(aio_write);
  nng_stream_dialer_close(dialer);
}

void webclient::send(const std::string &data) {
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

void webclient::recv_cb() {
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

void webclient::start_recv() {
  nng_iov iov{buffer.data(), buffer.size()};
  nng_aio_set_iov(aio_read, 1, &iov);
  nng_stream_recv(stream, aio_read);
}

std::shared_ptr<http_server> create_server(const std::string url) { return std::make_shared<http_server>(url); }
std::shared_ptr<webclient> connect_server(const std::string &uri,
                                          std::function<void(webclient &, const std::string &)> on_message,
                                          std::function<void(webclient &)> on_open,
                                          std::function<void(webclient &, const std::string &)> on_error,
                                          std::function<void(webclient &)> on_close, const bool is_text_mode) {
  return std::make_shared<webclient>(uri, on_message, on_open, on_error, on_close, is_text_mode);
}
*/
} // namespace kungfu::yijinjing::webserver