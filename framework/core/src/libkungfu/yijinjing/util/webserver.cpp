#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>
#include <memory>
#include <utility>
#include <algorithm>

using namespace kungfu::yijinjing::data;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing::journal;
using namespace std::literals;

namespace kungfu::yijinjing::webserver {
constexpr uint64_t PAGE_SIZE = 256;

stream::stream(nng_stream *s, bool is_server) : stream_id_(generate_stream_id(s, is_server)) {
  SPDLOG_DEBUG("stream");
  auto group = is_server ? "webserver"s : "webclient"s;
  location_ = location::make_shared(mode::LIVE, category::SYSTEM, group, std::to_string(stream_id_),
                                    std::make_shared<locator>(mode::LIVE));
  writer_ = std::make_shared<writer>(location_, location::PUBLIC, false, std::make_shared<noop_publisher>(), true,
                                     std::make_shared<bus>(false), PAGE_SIZE);
}

stream::~stream() {
  SPDLOG_DEBUG("~stream");
  close_data();
  writer_.reset();
  location_.reset();
}

uint64_t stream::get_stream_id(){
  return stream_id_;
};


void stream::close_data() {
  if (current_frame_) {
    writer_->close_frame_lock_free(1024);
    current_frame_.reset();
  }
}

void stream::open_data(nng_iov &iov) {
  current_frame_ = writer_->open_frame_lock_free(time::now_in_nano(), 10001000, 1024);
  iov.iov_buf = const_cast<void *>(current_frame_->data_address());
  iov.iov_len = current_frame_->data_length();
}

const yijinjing::data::location_ptr &stream::get_location() const { return location_; }

session::session(web_agent_ptr agent, nng_stream *s, bool is_server)
    : stream(s, is_server), stream_(s, nng_stream_free), agent_(std::move(agent)) {
  int rv;
  if ((rv = nng_aio_alloc(
           &aio_recv_,
           [](void *arg) {
             auto *pThis = reinterpret_cast<session *>(arg);
             pThis->recv_cb();
           },
           this)) != 0) {
    fatal("nng_aio_alloc read", rv);
  }

  if ((rv = nng_aio_alloc(
           &aio_send_,
           [](void *arg) {
             auto *pThis = reinterpret_cast<session *>(arg);
             pThis->send_cb();
           },
           this)) != 0) {
    fatal("nng_aio_alloc read", rv);
  }
  start_recv();
}

session::~session(){
  nng_aio_cancel(aio_recv_);
  nng_aio_wait(aio_recv_);
  nng_aio_cancel(aio_send_);
  nng_aio_wait(aio_send_);
  stream_.reset();
}

int session::send(const char *data, int len){
  nng_iov iov{};
  iov.iov_buf = (void *)data;
  iov.iov_len = len;

  if (nng_aio_busy(aio_send_)) {
    nng_aio_wait(aio_send_);
  }
  int rv = nng_aio_set_iov(aio_send_, 1, &iov);
  if (rv != 0) {
    fatal("nng_aio_set_iov", rv);
    return rv;
  }
  nng_stream_send(stream_, aio_send_);
  return 0;
}

void session::recv_cb() {
  int rv = nng_aio_result(aio_recv_);

  auto len = nng_aio_count(aio_recv_);
  switch (rv) {
  case 0: {
    start_recv();
    break;
  }
  case NNG_ECLOSED: {
    SPDLOG_DEBUG("NNG_ECLOSED");
    agent_->onDisconnect();
    //close_data();
    //writer_->mark(time::now_in_nano(), NngDisconnect::tag);
    //dispose_func_();
    break;
  }
  default:
    //such as NNG_ECANCELED by nng_cancel
    SPDLOG_DEBUG("default:{}", rv);
    agent_->onDisconnect();
    break;
  }
}

void session::send_cb() {
  int rv = nng_aio_result(aio_send_);
  if (rv) {
    agent_->onError();
    fatal("stream_send_cb", rv);
  }
}

void session::start_recv() {
  close_data();
  nng_iov iov{};
  open_data(iov);
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(stream_, aio_recv_);
}

websocket_client::websocket_client(const std::string &address, const bool is_text_mode, const bool tcp_no_delay)
    : tcp_no_delay_(tcp_no_delay_) {
  SPDLOG_DEBUG("websocket_client");
  int rv;
  nng_smart_ptr<nng_url> url{nng_url_free};
  rv = nng_url_parse(&url, address.c_str());
  if (rv != 0) {
    fatal("nng_url_parse", rv);
  }
  rv = nng_stream_dialer_alloc_url(&dialer_, url);
  if (rv != 0) {
    fatal("nng_stream_dialer_alloc", rv);
  }
  rv = nng_aio_alloc(&aio_dialer_, nullptr, nullptr);
  if (rv != 0) {
    fatal("nng_aio_alloc", rv);
  }

  if (is_text_mode) {
    nng_stream_dialer_set_bool(dialer_, NNG_OPT_WS_RECV_TEXT, true);
    nng_stream_dialer_set_bool(dialer_, NNG_OPT_WS_SEND_TEXT, true);
  }
}

websocket_client::~websocket_client() {
  SPDLOG_DEBUG("~websocket_client");
  //reader_.reset();
  session_.reset();
}

uint64_t websocket_client::get_stream_id(){
  return session_->get_stream_id();
};

void websocket_client::start() {
  nng_stream_dialer_dial(dialer_, aio_dialer_);
  nng_aio_wait(aio_dialer_);
  int rv;
  rv = nng_aio_result(aio_dialer_);
  if (rv != 0) {
    fatal("dial", rv);
  }
  auto *stream = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_dialer_, 0));
  // disable Nagle, send-msg low-latency
  if (tcp_no_delay_) {
    nng_stream_set_bool(stream, NNG_OPT_TCP_NODELAY, true);
  }
  auto self{this->shared_from_this()};
  session_ = std::make_shared<session>(self, stream, false);
  //  reader_->join(session_->get_location(), location::PUBLIC, time::now_in_nano());
  add_join(session_->get_location(), location::PUBLIC, time::now_in_nano());
  onConnect();
}
void websocket_client::stop(){
  // have been released
  if(!aio_dialer_ && !dialer_){
    onError();
    return ;
  }

  if(aio_dialer_)
  {
    nng_aio_cancel(aio_dialer_);
    nng_aio_wait(aio_dialer_);
    aio_dialer_.reset();
  }
  if (dialer_) {
    nng_stream_dialer_close(dialer_);
    dialer_.reset();
  }
  onDisconnect();
  return ;
}

int websocket_client::send(const char *data, int data_len) {
  return session_->send(data, data_len);
}

void websocket_client::onError() { SPDLOG_ERROR("websocket_client onError"); }

void websocket_client::onDisconnect() { SPDLOG_CRITICAL("websocket_client onDisconnect"); }

void websocket_client::onConnect() { SPDLOG_INFO("websocket_client onConnect"); }

websocket_server::websocket_server(const nng_url *base_url, std::string path, const bool is_text_mode,
                                   const bool tcp_no_delay, const size_t session_max)
    : url_(base_url), is_text_mode_(is_text_mode), tcp_no_delay_(tcp_no_delay), session_max_(session_max),
      session_num_(0) {
  SPDLOG_DEBUG("websocket_server");

  int rv;
  if ((rv = nng_aio_alloc(
           &aio_listener_, [](void *arg) { ((websocket_server *)arg)->accept_cb(); }, this)) != 0) {
    fatal("nng_aio_alloc", rv);
  }
  nng_url url = *url_;
  const bool secure = (strcmp(url_->u_scheme, "https") == 0);
  url.u_path = (char *)path.c_str();
  url.u_scheme = (char *)(secure ? "wss" : "ws");
  rv = nng_stream_listener_alloc_url(&listener_, &url);
  if (rv) {
    fatal("nng_stream_listener_alloc_url", rv);
  }

  nng_stream_listener_set_bool(listener_, NNG_OPT_TCP_NODELAY, true);
  nng_stream_listener_set_bool(listener_, NNG_OPT_TCP_KEEPALIVE, true);
  nng_stream_listener_set_size(listener_, NNG_OPT_WS_SENDMAXFRAME, 1000000);

  if (is_text_mode_) {
    nng_stream_listener_set_bool(listener_, NNG_OPT_WS_SEND_TEXT, true);
    nng_stream_listener_set_bool(listener_, NNG_OPT_WS_RECV_TEXT, true);
  }
}

websocket_server::~websocket_server() {
  //nng_url_free(url_);
  //reader_.reset();
  std::erase_if(sessions_, [](const auto &){return true;});
  SPDLOG_DEBUG("~websocket_server");
}

void websocket_server::start() {
  int rv;
  rv = nng_stream_listener_listen(listener_);
  if (rv) {
    fatal("nng_stream_listener_listen", rv);
  }

  start_accept();
}

void websocket_server::stop() {
  // have been released
  if(!aio_listener_ && !listener_){
    onError();
    return ;
  }

  if(aio_listener_)
  {
    nng_aio_cancel(aio_listener_);
    nng_aio_wait(aio_listener_);
    aio_listener_.reset();
  }
  if (listener_) {
    nng_stream_listener_close(listener_);
    listener_.reset();
  }
  onDisconnect();
  return ;
}

void websocket_server::start_accept() { nng_stream_listener_accept(listener_, aio_listener_); }

void websocket_server::accept_cb() {
  int rv = nng_aio_result(aio_listener_);
  /*May be should deal aio_result,
    If a client dial to server,
    and the server received, but closed when process data
  */  
  if (rv != 0) {
    return;
  }

  auto *stream = reinterpret_cast<nng_stream *>(nng_aio_get_output(aio_listener_, 0));

  // disable Nagle, send-msg low-latency
  if (tcp_no_delay_) {
    nng_stream_set_bool(stream, NNG_OPT_TCP_NODELAY, true);
  }
  if (session_max_ > 0 && (session_num_ + 1) >= session_max_) {
    SPDLOG_CRITICAL("connection limited");
    stop();
  } else {
    start_accept();
  }
  add_session(stream);
  session_num_++;
}

void websocket_server::send(const char *data, int len, uint64_t session_id){};

void websocket_server::add_session(nng_stream *stream) {
  SPDLOG_DEBUG("add_session");
  auto session_id = generate_stream_id(stream, true);
  auto self{this->shared_from_this()};
  auto session_p = std::make_shared<session>(self, stream, true);
  std::unique_lock<std::shared_mutex> lock(sessions_mtx_);
  sessions_.emplace(session_id, session_p);
  //  reader_->join(session_p->get_location(), location::PUBLIC, time::now_in_nano());
  add_join(session_p->get_location(), location::PUBLIC, time::now_in_nano());
}

void websocket_server::remove_session(uint64_t session_id) {
  SPDLOG_DEBUG("remove_session:{}", session_id);
  std::unique_lock<std::shared_mutex> lock(sessions_mtx_);

  if (!sessions_.contains(session_id)) {
    SPDLOG_ERROR("stream not exist!");
    return;
  }
  // auto session_location = location::make_shared(mode::LIVE, category::SYSTEM, "webserver"s,
  // std::to_string(session_id),std::make_shared<locator>(mode::LIVE));
  auto session_location = sessions_.at(session_id)->get_location();
  //  reader_->disjoin(session_location, location::PUBLIC);
  add_disjion(session_location, location::PUBLIC);
  sessions_.erase(session_id);
}

void websocket_server::onError() { SPDLOG_ERROR("websocket_server onError"); }
void websocket_server::onDisconnect() { SPDLOG_CRITICAL("websocket_server onDisconnect"); }
void websocket_server::onConnect() { SPDLOG_INFO("websocket_server onConnect"); }

http_server::http_server(const std::string &address) : started_(false) {
  SPDLOG_DEBUG("http_server");
  int rv;
  if ((rv = nng_url_parse(&url_, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_http_server_hold(&server_, url_)) != 0) {
    fatal("nng_http_server_hold", rv);
  }
}

http_server::~http_server() { SPDLOG_DEBUG("~http_server"); }

void http_server::add_websocket(const std::string &path, bool is_text_mode, bool tcp_no_delay,
                                const size_t session_max) {
  auto websocket = std::make_shared<websocket_server>(url_, path, is_text_mode, tcp_no_delay, session_max);
  websocket->start();
  websockets_.emplace(std::make_pair(path, std::move(websocket)));
}

void http_server::remove_websocket(const std::string &path) {
  if(websockets_.contains(path)){
    auto websocket = websockets_.at(path);
    websocket->stop();
    websockets_.erase(path);
    return ;
  }
  onError();
  return ;
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
  std::erase_if(websockets_, [](const auto &){return true;});
  if (server_ != nullptr) {
    nng_http_server_stop(server_);
    server_.reset();
  }
  url_.reset();
}

void http_server::onError() { SPDLOG_ERROR("http_server onError"); }

void http_server::onDisconnect() { SPDLOG_CRITICAL("http_server onDisconnect"); }

void http_server::onConnect() { SPDLOG_INFO("http_server onConnect"); }

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

void web_agent::add_join(const kungfu::yijinjing::data::location_ptr &location, uint32_t dest, int64_t begin_time) {
  while (flag_write.test_and_set(std::memory_order_acquire)) {
  }
  SPDLOG_DEBUG("add_join");
  join_channels_.insert_or_assign({location, dest}, begin_time);
  flag_has.store(true, std::memory_order_release);
  flag_write.clear(std::memory_order_acquire);
  SPDLOG_DEBUG("end add_join:{} flag_write:{}",flag_has,flag_write);
}

void web_agent::add_disjion(const location_ptr &location, uint32_t dest) {
  while (flag_write.test_and_set(std::memory_order_acquire)) {
  }
  SPDLOG_DEBUG("add_disjion");
  disjoin_channels_.insert({location, dest});
  flag_has.store(true, std::memory_order_release);
  flag_write.clear(std::memory_order_acquire);
  SPDLOG_DEBUG("end add_join:{} flag_write:{}",flag_has,flag_write);
}

void web_agent::cleanup_reader_disjoin() {
  SPDLOG_DEBUG("cleanup_reader_disjoin");
  for (const auto &pair : disjoin_channels_) {
    SPDLOG_DEBUG("disjion");
    reader_->disjoin(pair.first, pair.second);
  }
  disjoin_channels_.clear();
}

void web_agent::cleanup_reader_join() {
  SPDLOG_DEBUG("cleanup_reader_disjoin");
  for (const auto &pair : join_channels_) {
    SPDLOG_DEBUG("disjion");
    reader_->join(pair.first.first, pair.first.second, pair.second);
  }
  join_channels_.clear();
}

web_agent::web_agent() : reader_(std::make_shared<reader>(true, true, std::make_shared<bus>(false))) {}

void web_agent::on_frame() {
  SPDLOG_DEBUG("start on_frame:{}",flag_has);
  if (flag_has.load()) {
    SPDLOG_DEBUG("flag_has");
    while (flag_write.test_and_set(std::memory_order_acquire)) {
    }
    SPDLOG_DEBUG("flag_write");
    cleanup_reader_join();
    cleanup_reader_disjoin();
    flag_has.store(false, std::memory_order_release);
    flag_write.clear(std::memory_order_acquire);
  }
}
bool web_agent::data_available() { return reader_->data_available(); }

void web_agent::next() {
  on_frame();
  reader_->next();
}

} // namespace kungfu::yijinjing::webserver
