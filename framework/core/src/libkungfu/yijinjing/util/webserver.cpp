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
using namespace std::literals;

namespace kungfu::yijinjing::webserver {
constexpr uint64_t PAGE_SIZE = 256;

stream::stream(nng_stream *s, bool is_server)
    : stream_id_(generate_stream_id(s,is_server)){
  SPDLOG_DEBUG("stream");
  auto group = is_server?"webserver"s:"webclient"s;
  location_ = location::make_shared(mode::LIVE, category::SYSTEM, group, std::to_string(stream_id_),
                                    std::make_shared<locator>(mode::LIVE));
  writer_ = std::make_shared<writer>(location_, location::PUBLIC, false, std::make_shared<noop_publisher>(), true,
                                     std::make_shared<bus>(false), PAGE_SIZE);

}

stream::~stream() {
  SPDLOG_DEBUG("~stream");
  close_data();
}

uint64_t stream::get_stream_id() const { return stream_id_; }

void stream::close_data() {
  if (current_frame_) {
    writer_->close_frame_lock_free(1024);
    current_frame_.reset();
  }
  return ;
}

void stream::open_data(nng_iov& iov){
  current_frame_ = writer_->open_frame_lock_free(time::now_in_nano(), 10001000, 1024);
  iov.iov_buf = const_cast<void *>(current_frame_->data_address());
  iov.iov_len = current_frame_->data_length();
}


const yijinjing::data::location_ptr &stream::get_location() const { return location_; }

session::session(web_agent_ptr agent, nng_stream *s, bool is_server):stream(s,is_server),stream_(s,nng_stream_free),agent_(agent){
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

  if ((rv = nng_aio_alloc(&aio_send_, [](void *arg) {
             auto *pThis = reinterpret_cast<session *>(arg);
             pThis->send_cb();
           }, this)) != 0) {
    fatal("nng_aio_alloc read", rv);
  }
  start_recv();
}

session::~session(){
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

void session::recv_cb(){
  int rv = nng_aio_result(aio_recv_);

  auto len = nng_aio_count(aio_recv_);
  switch (rv) {
  case 0: {
    start_recv();
    break;
  }
  case NNG_ECLOSED: {
    SPDLOG_DEBUG("NNG_ECLOSED");
    //close_data();
    //writer_->mark(time::now_in_nano(), NngDisconnect::tag);
    //dispose_func_();
    break;
  }
  default:
    SPDLOG_DEBUG("default:{}", rv);
    break;
  }
  return ;
}

void session::send_cb(){
  int rv = nng_aio_result(aio_send_);
  if(rv){
    agent_->onError();
    fatal("stream_send_cb",rv);
  }
  return;
}

void session::start_recv() {
  close_data();
  nng_iov iov{};
  open_data(iov);
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(stream_, aio_recv_);
  return ;
}

websocket_client::websocket_client(const std::string &address,const bool is_text_mode, const bool tcp_no_delay):tcp_no_delay_(tcp_no_delay_),reader_(std::make_shared<reader>(true, true, std::make_shared<bus>(false))){
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

websocket_client::~websocket_client() { SPDLOG_DEBUG("~websocket_client"); }

void websocket_client::start(){
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
  reader_->join(session_->get_location(), location::PUBLIC, time::now_in_nano());
  onConnect();
}
void websocket_client::stop(){
  onDisconnect();
  //nng_stream_dialer_free(dialer_); 
}

int websocket_client::send(const char *data, int data_len) { return session_->send(data, data_len); }

void websocket_client::onError(){
  SPDLOG_ERROR("websocket_client onError");
}

void websocket_client::onDisconnect(){
  SPDLOG_CRITICAL("websocket_client onDisconnect");
}

void websocket_client::onConnect(){
  SPDLOG_INFO("websocket_client onConnect");
}


websocket_server::websocket_server(const nng_url *base_url, std::string path,
                                   const bool is_text_mode, const bool tcp_no_delay, const size_t session_max)
    : url_(base_url), is_text_mode_(is_text_mode), tcp_no_delay_(tcp_no_delay),
      session_max_(session_max), session_num_(0),reader_(std::make_shared<reader>(true, true, std::make_shared<bus>(false))) {
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
}

websocket_server::~websocket_server() {
  SPDLOG_DEBUG("~websocket_server");
  nng_aio_cancel(aio_listener_);
  aio_listener_.reset();

  if (listener_ != nullptr) {
    nng_stream_listener_close(listener_);
    listener_.reset();
  }
  //stop();
}

void websocket_server::start() {
  int rv;
  if (rv = nng_stream_listener_listen(listener_)) {
    fatal("nng_stream_listener_listen", rv);
  }

  start_accept();
  return ;
}

void websocket_server::stop() {
  return ;
}

void websocket_server::start_accept() { 
  nng_stream_listener_accept(listener_, aio_listener_);
  return ;
}

void websocket_server::accept_cb() {
  int rv = nng_aio_result(aio_listener_);
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
  return ;
}

void websocket_server::send(const char *data, int len, uint64_t session_id){};

void websocket_server::add_session(nng_stream *stream){
  SPDLOG_DEBUG("add_session");
  auto session_id = generate_stream_id(stream,true);
  auto self{this->shared_from_this()};
  auto session_p = std::make_shared<session>(self, stream, true);
  std::unique_lock<std::shared_mutex> lock(sessions_mtx_);
  sessions_.emplace(session_id, session_p);
  reader_->join(session_p->get_location(), location::PUBLIC, time::now_in_nano());
  return ;
}

void websocket_server::remove_session(uint64_t session_id){
  SPDLOG_DEBUG("remove_session:{}",session_id);
  std::unique_lock<std::shared_mutex> lock(sessions_mtx_);
  
  if(!sessions_.contains(session_id)){
    SPDLOG_ERROR("stream not exist!");
    return ;
  }
  //auto session_location = location::make_shared(mode::LIVE, category::SYSTEM, "webserver"s, std::to_string(session_id),std::make_shared<locator>(mode::LIVE));
  auto session_location = sessions_.at(session_id)->get_location();
  reader_->disjoin(session_location, location::PUBLIC);
  sessions_.erase(session_id);
  return;
}

void websocket_server::onError(){
  SPDLOG_ERROR("websocket_server onError");
}
void websocket_server::onDisconnect(){
  SPDLOG_CRITICAL("websocket_server onDisconnect");
}
void websocket_server::onConnect(){
  SPDLOG_INFO("websocket_server onConnect");
}

http_server::http_server(const std::string &address)
    : started_(false) {
  SPDLOG_DEBUG("http_server");
  int rv;
  if ((rv = nng_url_parse(&url_, address.c_str())) != 0) {
    fatal("nng_url_parse", rv);
  }
  if ((rv = nng_http_server_hold(&server_, url_)) != 0) {
    fatal("nng_http_server_hold", rv);
  }
}

http_server::~http_server() {
  SPDLOG_DEBUG("~http_server");
}

void http_server::add_websocket(const std::string &path, bool is_text_mode, bool tcp_no_delay,
                                const size_t session_max) {
  auto websocket = std::make_shared<websocket_server>(url_, path, is_text_mode, tcp_no_delay,
                                                      session_max);
  websocket->start();
  websockets_.emplace(std::make_pair(path, std::move(websocket)));
  return ;
}

void http_server::remove_websocket(const std::string &path) {
  auto it = websockets_.find(path);
  if (it != websockets_.end()) {
    websockets_.erase(path);
  }
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
  return ;
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
  return ;
}

void http_server::onError(){
  SPDLOG_ERROR("http_server onError");
}

void http_server::onDisconnect(){
  SPDLOG_CRITICAL("http_server onDisconnect");
}

void http_server::onConnect(){
  SPDLOG_INFO("http_server onConnect");
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

} // namespace kungfu::yijinjing::webserver
