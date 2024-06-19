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

uint64_t roundup_pow_of_two(const uint64_t x) {
  if (x == 0)
    return 0;
  if (x == 1)
    return 2;
  uint64_t ret = 1;
  while (ret < x) {
    ret = ret << 1;
  }
  return ret;
}

stream::stream(nng_stream *s, uint64_t stream_id, uint64_t aio_nums)
    : stream_(s, nng_stream_free), stream_id_(stream_id) {
  SPDLOG_DEBUG("stream");
  aio_nums_ = roundup_pow_of_two(aio_nums);
  location_ = location::make_shared(mode::LIVE, category::SYSTEM, "webserver", std::to_string(stream_id),
                                    std::make_shared<locator>(mode::LIVE));
  writer_ = std::make_shared<writer>(location_, location::PUBLIC, false, std::make_shared<noop_publisher>(), true,
                                     std::make_shared<bus>(false), PAGE_SIZE);

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
    fatal("nng_aio_alloc read", rv);
  }
  /*
  aio_send_.reserve(aio_nums_);
  for (int i = 0; i < aio_nums_; i++) {
    if ((rv = nng_aio_alloc(&aio_send_[i], [](void *arg) {
             auto *pThis = reinterpret_cast<stream *>(arg);
             pThis->stream_send_cb();
           }, this)) != 0) {
      fatal("nng_aio_alloc write", rv);
    }
  }
  cur_index_ = 0;
  */
  start_recv();
}

stream::~stream() {
  SPDLOG_DEBUG("~stream");
  close_data();
  cancel();
  nng_stream_close(stream_);
  // nng_stream_free(stream_);
  // nng_aio_free(aio_recv_);
  // for (int i = 0; i < aio_nums_; i++) {
  // nng_aio_free(aio_send_[i]);
  //}
}

uint64_t stream::get_stream_id() const { return stream_id_; }

uint64_t stream::get_opposite_stream_id() {
  nng_sockaddr local_address, remote_address;
  nng_stream_get_addr(stream_, NNG_OPT_REMADDR, &remote_address);
  nng_stream_get_addr(stream_, NNG_OPT_LOCADDR, &local_address);
  return (static_cast<uint64_t>(local_address.s_in.sa_addr) << 32) |
         (static_cast<uint64_t>(local_address.s_in.sa_port) << 16) | remote_address.s_in.sa_port;
}

void stream::close_data() {
  if (current_frame_) {
    writer_->close_frame_lock_free(1024);
    current_frame_.reset();
  }
}

void stream::start_recv() {
  close_data();
  current_frame_ = writer_->open_frame_lock_free(time::now_in_nano(), 10001000, 1024);
  nng_iov iov{const_cast<void *>(current_frame_->data_address()), current_frame_->data_length()};
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(stream_, aio_recv_);
}

void stream::stream_recv_cb() {
  int rv = nng_aio_result(aio_recv_);

  auto len = nng_aio_count(aio_recv_);
  switch (rv) {
  case 0: {
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

void stream::stream_send_cb() {}

/*
int stream::stream_send(const std::string &data) {
  nng_iov iov;
  iov.iov_buf = (void *)data.data();
  iov.iov_len = data.size();
  if (nng_aio_busy(aio_send_[cur_index_])) {
    nng_aio_wait(aio_send_[cur_index_]);
  }
  int rv = nng_aio_set_iov(aio_send_[cur_index_], 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(stream_, aio_send_[cur_index_]);
  // a%b equal to a&(b-1), if b == 2^n
  cur_index_ = (cur_index_ + 1) & (aio_nums_ - 1);

  return 0;
}

int stream::stream_send(const char *data, const int len) {
  nng_iov iov;
  iov.iov_buf = (void *)data;
  iov.iov_len = len;
  if (nng_aio_busy(aio_send_[cur_index_])) {
    nng_aio_wait(aio_send_[cur_index_]);
  }

  int rv = nng_aio_set_iov(aio_send_[cur_index_], 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(stream_, aio_send_[cur_index_]);
  // a%b equal to a&(b-1), if b == 2^n
  cur_index_ = (cur_index_ + 1) & (aio_nums_ - 1);
  return 0;
}


int stream::stream_send(const std::string &data) {
  nng_iov iov;
  iov.iov_buf = (void *)data.data();
  iov.iov_len = data.size();
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(stream_, aio_send_);
  nng_aio_wait(aio_send_);
  rv = nng_aio_result(aio_send_);
  if (rv != 0) {
    fatal("nng_aio_result", rv);
  }
  return rv;
}
*/

int stream::stream_send(const std::string &data) {
  nng_iov iov;
  iov.iov_buf = (void *)data.data();
  iov.iov_len = data.size();

  if (nng_aio_busy(aio_send_)) {
    nng_aio_wait(aio_send_);
  }
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(stream_, aio_send_);
  return 0;
}

int stream::stream_send(const char *data, const int len) {
  nng_iov iov;
  iov.iov_buf = (void *)data;
  iov.iov_len = len;

  if (nng_aio_busy(aio_send_)) {
    nng_aio_wait(aio_send_);
  }
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
  }
  nng_stream_send(stream_, aio_send_);
  return 0;
}

void stream::cancel() {
  nng_aio_cancel(aio_recv_);
  nng_aio_wait(aio_recv_);
  nng_aio_cancel(aio_send_);
  nng_aio_wait(aio_send_);
  /*
  for (int i = 0; i < aio_nums_; i++) {
    nng_aio_cancel(aio_send_[i]);
    nng_aio_wait(aio_send_[i]);
  }
  */
}

const yijinjing::data::location_ptr &stream::get_location() const { return location_; }

websocket_server::websocket_server(stream_manage_ptr stream_manager, const nng_url *base_url, std::string path,
                                   const bool is_text_mode, const bool tcp_no_delay, const size_t max_num_connections)
    : web_agent(std::move(stream_manager)), url_(base_url), is_text_mode_(is_text_mode), tcp_no_delay_(tcp_no_delay),
      max_num_connections_(max_num_connections), num_connected_(0) {
  SPDLOG_DEBUG("websocket_server");

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_accept_, [](void *arg) { ((websocket_server *)arg)->accept_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  nng_url url = *url_;
  const bool secure = (strcmp(url_->u_scheme, "https") == 0);
  url.u_path = (char *)path.c_str();
  url.u_scheme = (char *)(secure ? "wss" : "ws");
  if (rv = nng_stream_listener_alloc_url(&listener_, &url)) {
    fatal("nng_stream_listener_alloc_url", rv);
  }

  nng_stream_listener_set_bool(listener_, NNG_OPT_TCP_NODELAY, true);
  nng_stream_listener_set_bool(listener_, NNG_OPT_TCP_KEEPALIVE, true);
  nng_stream_listener_set_size(listener_, NNG_OPT_WS_SENDMAXFRAME, 1000000);

  if (is_text_mode_) {
    nng_stream_listener_set_bool(listener_, NNG_OPT_WS_SEND_TEXT, true);
    nng_stream_listener_set_bool(listener_, NNG_OPT_WS_RECV_TEXT, true);
  }

  start();
}

/*
websocket_server::websocket_server(stream_manage_ptr stream_manager, const char *url,
                     const bool is_text_mode, const bool tcp_no_delay, const size_t max_num_connections)
    : stream_manager_(std::move(stream_manager)),
      is_text_mode_(is_text_mode), tcp_no_delay_(tcp_no_delay), max_num_connections_(max_num_connections),
      num_connected_(0) {
  SPDLOG_DEBUG("websocket_server");

  int rv;
  nng_url url = *url_;
  if ((rv = nng_url_parse(&url_, url.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_http_server_hold(&server_, url_)) != 0) {
    fatal("nng_http_server_hold", rv);
  }

  if ((rv = nng_aio_alloc(
           &aio_accept_, [](void *arg) { ((websocket_server *)arg)->accept_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }

  int rv = nng_stream_listener_alloc_url(&listener_, &url);
  if (rv != 0) {
    fatal("nng_stream_listener_alloc_url", rv);
  }
  start();
}
*/

websocket_server::~websocket_server() {
  SPDLOG_DEBUG("~websocket_server");
  stop();
}

void websocket_server::start() {
  int rv;
  if (rv = nng_stream_listener_listen(listener_)) {
    fatal("nng_stream_listener_listen", rv);
  }

  start_accept();
}

void websocket_server::stop() {
  nng_aio_cancel(aio_accept_);
  aio_accept_.reset();

  if (listener_ != nullptr) {
    nng_stream_listener_close(listener_);
    listener_.reset();
  }
}

void websocket_server::start_accept() { nng_stream_listener_accept(listener_, aio_accept_); }

void websocket_server::accept_cb() {
  int rv = nng_aio_result(aio_accept_);
  if (rv != 0) {
    return;
  }

  auto *stream = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_accept_, 0));

  // disable Nagle, send-msg low-latency
  if (tcp_no_delay_) {
    nng_stream_set_bool(stream, NNG_OPT_TCP_NODELAY, true);
  }

  try {
    if (max_num_connections_ > 0 && (num_connected_ + 1) >= max_num_connections_) {
      SPDLOG_CRITICAL("connection limited");
      stop();
    } else {
      start_accept();
    }
    get_stream_manager()->add_stream(stream);
    num_connected_++;
  } catch (std::exception &) {
  }
}

void websocket_server::publish(const char *data, int len, uint64_t stream_id) {
  get_stream_manager()->publish(stream_id, data, len);
  return;
}

http_server::http_server(const std::string &address)
    : web_agent(std::make_shared<kungfu::yijinjing::webserver::stream_manage>()), started_(false) {
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
  stop();
}

void http_server::add_websocket(const std::string &path, bool is_text_mode, bool tcp_no_delay,
                                const size_t max_num_connections) {
  auto websocket = std::make_shared<websocket_server>(get_stream_manager(), url_, path, is_text_mode, tcp_no_delay,
                                                      max_num_connections);
  websockets_.emplace(std::make_pair(path, std::move(websocket)));
}

void http_server::remove_websocket(const std::string &path) {
  auto it = websockets_.find(path);
  if (it != websockets_.end()) {
    websockets_.erase(path);
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

void http_server::stop() {
  if (!started_) {
    return;
  }
  started_ = false;
  if (!websockets_.empty()) {
    SPDLOG_ERROR("Websocket service still run!");
    // TODO: should do more to release memory?
  }
  if (server_ != nullptr) {
    nng_http_server_stop(server_);
    server_.reset();
  }
  url_.reset();
}

void http_server::publish(const char *data, int len, uint64_t stream_id) {
  get_stream_manager()->publish(stream_id, data, len);
  return;
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

websocket_client::websocket_client(stream_manage_ptr stream_manager, const std::string &address,
                                   const bool is_text_mode, const bool tcp_no_delay) {
  SPDLOG_DEBUG("websocket_client");
  int rv;
  nng_smart_ptr<nng_url> url{nng_url_free};
  if ((rv = nng_url_parse(&url, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_stream_dialer_alloc_url(&dialer_, url)) != 0) {
    fatal("nng_stream_dialer_alloc", rv);
  }
  if ((rv = nng_aio_alloc(&aio_dialer_, nullptr, nullptr)) != 0) {
    fatal("nng_aio_alloc", rv);
  }

  if (is_text_mode) {
    nng_stream_dialer_set_bool(dialer_, NNG_OPT_WS_RECV_TEXT, true);
    nng_stream_dialer_set_bool(dialer_, NNG_OPT_WS_SEND_TEXT, true);
  }

  nng_stream_dialer_dial(dialer_, aio_dialer_);
  nng_aio_wait(aio_dialer_);
  rv = nng_aio_result(aio_dialer_);
  if (rv != 0) {
    fatal("dial", rv);
  }
  auto *s = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_dialer_, 0));
  // disable Nagle, send-msg low-latency
  if (tcp_no_delay) {
    nng_stream_set_bool(s, NNG_OPT_TCP_NODELAY, true);
  }
  auto temp_stream = std::make_shared<stream>(s, generate_stream_id(s));
  stream_ = temp_stream;
}

websocket_client::~websocket_client() { SPDLOG_DEBUG("~websocket_client"); }

uint64_t websocket_client::get_stream_id() { return stream_->get_stream_id(); }

int websocket_client::send_msg(const char *data, int data_len) { return stream_->stream_send(data, data_len); }

stream_ptr websocket_client::get_stream() { return stream_; }

void websocket_client::start() {}

void websocket_client::stop() { nng_stream_dialer_free(dialer_); }

void websocket_client::publish(const char *data, int len, uint64_t stream_id) {
  // send_msg(data, len);
  stream_->stream_send(data, len);
  return;
}

int stream_manage::publish(uint64_t stream_id, const std::string &msg) {
  auto stream_ptr = get_stream_by_id(stream_id);
  if (stream_ptr == nullptr) {
    SPDLOG_ERROR("Publish Failed");
    return -1;
  }
  stream_ptr->stream_send(msg);
  return 0;
}
int stream_manage::publish(uint64_t stream_id, const char *data, const int len) {
  auto stream_ptr = get_stream_by_id(stream_id);
  if (stream_ptr == nullptr) {
    SPDLOG_ERROR("Publish Failed");
    return -1;
  }
  stream_ptr->stream_send(data, len);
  return 0;
}

stream_ptr stream_manage::get_stream_by_id(uint64_t stream_id) {
  std::shared_lock<std::shared_mutex> lock(mtx_);
  if (!streams_.contains(stream_id)) [[unlikely]] {
    SPDLOG_ERROR("do not exist stream_id:{}", stream_id);
    return nullptr;
  } else {
    return streams_.at(stream_id);
  }
}

void stream_manage::add_stream(nng_stream *s) {
  SPDLOG_DEBUG("add_stream");
  auto temp_stream = std::make_shared<stream>(s, generate_stream_id(s));
  std::unique_lock<std::shared_mutex> lock(mtx_);
  streams_.emplace(temp_stream->get_stream_id(), temp_stream);
  reader_->join(temp_stream->get_location(), location::PUBLIC, time::now_in_nano());
  location_to_stream_id_.insert_or_assign(temp_stream->get_location()->location_uid, temp_stream->get_stream_id());
}

void stream_manage::add_stream(const stream_ptr &s) {
  SPDLOG_DEBUG("add_stream");
  std::unique_lock<std::shared_mutex> lock(mtx_);
  streams_.emplace(s->get_stream_id(), s);
  reader_->join(s->get_location(), location::PUBLIC, time::now_in_nano());
  location_to_stream_id_.insert_or_assign(s->get_location()->location_uid, s->get_stream_id());
}

journal::reader_ptr &stream_manage::get_reader() { return reader_; }

stream_manage::stream_manage() { reader_ = std::make_shared<reader>(true, true, std::make_shared<bus>(false)); }

uint64_t stream_manage::get_stream_id(uint32_t location_uid) {
  std::shared_lock<std::shared_mutex> lock(mtx_);
  auto iter = location_to_stream_id_.find(location_uid);
  if (iter != location_to_stream_id_.end()) {
    return iter->second;
  }
  return 0;
}

} // namespace kungfu::yijinjing::webserver
