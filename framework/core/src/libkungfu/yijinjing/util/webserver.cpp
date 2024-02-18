#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>
#include <kungfu/yijinjing/nanomsg/socket.h>
#include <kungfu/yijinjing/journal/assemble.h>
#include <thread>

using namespace kungfu::yijinjing::nanomsg;
using namespace kungfu::yijinjing::data;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::journal;

namespace kungfu::yijinjing::webserver {
stream::stream(nng_stream *s, uint64_t stream_id, uint32_t buffer_size)
    : s_(s), stream_id_(stream_id), rec_buffer_(buffer_size) {
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
uint64_t stream::get_stream_id() { return stream_id_; }
void stream::start_recv() {
  SPDLOG_DEBUG("start start_recv");
  nng_iov iov = {rec_buffer_.data(), rec_buffer_.size()};
  nng_aio_set_iov(aio_recv_, 1, &iov);
  nng_stream_recv(s_, aio_recv_);
  SPDLOG_DEBUG("end start_recv");
}

void stream::thread_read_data(kungfu::longfist::types::RequestRemoteData request){
  if(!asm_read_){
    //location_ptr loc = std::make_shared<location>(,std::make_shared<remote_locator>());
    asm_read_ = std::make_shared<assemble>(std::make_shared<kungfu::yijinjing::data::locator>(),get_mode_name(request.mode),get_category_name(request.category),request.group,request.name);
  }
  else{
    auto data_read = asm_read_->read_datas(request.type,request.query_num,time::now_in_nano());
    for(auto data_pair:data_read){
      std::string data_str(data_pair.second.begin(), data_pair.second.end());
      stream_send(data_str);
    }
    RemoteDataEnd end;
    end.req_nums = asm_read_->get_num();
    stream_send(end.to_string());
    asm_read_->reset_num();
  }
}

void stream::stream_recv_cb() {
  SPDLOG_DEBUG("stream_recv_cb");
  int rv = nng_aio_result(aio_recv_);

  auto len = nng_aio_count(aio_recv_);
  switch (rv) { 
  case 0: {
    {
      //处理数据，Request/cancelorder/insertorder
      //落盘？发送？ 多个cancel?
      std::string data((char *)rec_buffer_.data(), len);
      // 1、如果是RequestRemote 则创建Assemble对象，然后发送数据
      // 2、如果是Insert/CancelOrder或其他信号，则放进缓冲区，由drain处理
      //后续可以判断OrderAction即时处理相关信号
      auto frame = std::make_shared<nanomsg_json>(data);
      if(frame->msg_type() == kungfu::longfist::types::RequestRemoteData::tag){
        auto data_str = frame->data_as_string();
        RequestRemoteData request_data(data_str.c_str(), data_str.length());
        thread_read_data(request_data);
        /*
        //will be blocked in this function
        auto f = std::async(std::launch::async, [&]{
          //judge is remote_locator?
          yijinjing::journal::assemble asm_read(std::move(locator()),request_data.mode,request_data.category,request_data.group,request_data.name);})
        */
      }
      /*
      else if(frame->msg_type() == kungfu::longfist::types::OrderAction::tag){
        get_writer()->write();
      }
      */
      else{
        std::lock_guard<std::mutex> lock(mtx_);
        data_received_.emplace_back((char *)rec_buffer_.data(), len);
      }
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

webserver::webserver(stream_manage_ptr stream_manager,const nng_url *base_url, const std::string &path, const bool is_text_mode,
                     const size_t max_num_connections)
    : stream_manager_(stream_manager),base_url_(base_url), path_(path), is_text_mode_(is_text_mode), max_num_connections_(max_num_connections),
      num_connected_(0) {
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

  nng_stream *s = (nng_stream *)nng_aio_get_output(aio_accept, 0);

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

http_server::http_server(const std::string address) {
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

void http_server::add_websocket(stream_manage_ptr stream_manager,const std::string &path, bool is_text_mode, const size_t max_num_connections) {
  auto websocket = std::shared_ptr<webserver>(new webserver(stream_manager, url_, path, is_text_mode, max_num_connections));
  const auto id = websockets_.empty() ? 1 : websockets_.rbegin()->first + 1;
  websockets_.emplace(std::make_pair(id, std::move(websocket)));
  return;
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
    throw std::runtime_error("http_server not started");
  }
  nng_sockaddr addr;
  int rv;
  if ((rv = nng_http_server_get_addr(server_, &addr)) != 0) {
    fatal("nng_http_server_get_port", rv);
  }
  return ntohs(addr.s_in.sa_port);
}

webclient::webclient(stream_manage_ptr stream_manager, const std::string &address, std::function<void(webclient &, const std::string &)> message,
                     std::function<void(webclient &)> open, std::function<void(webclient &, const std::string &)> error,
                     std::function<void(webclient &)> close, const bool is_text_mode)
    : stream_manager_(stream_manager), on_message(message), on_open(open), on_error(error), on_close(close) {
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
  nng_stream *s = (nng_stream *)nng_aio_get_output(aio_dialer, 0);
  auto temp_stream = std::make_shared<stream>(s,generate_stream_id(s));
  stream_ = temp_stream;
  stream_manager_->add_stream(temp_stream);
}

webclient::~webclient() {
SPDLOG_DEBUG("~webclient");
}

uint64_t webclient::get_stream_id(){return stream_->get_stream_id();}
stream_manage::stream_manage() {}
stream_manage::~stream_manage() {}

int stream_manage::publish(uint64_t stream_id, const std::string &msg) {
  if (!streams_.contains(stream_id)) {
    return -1;
  }
  streams_.at(stream_id)->stream_send(msg);
  return 0;
}
std::vector<std::string> &stream_manage::get_notice(uint64_t stream_id) {
  return streams_.find(stream_id)->second->data_received_;
}
stream_ptr stream_manage::get_stream_by_id(uint64_t stream_id) { return streams_.find(stream_id)->second; }
std::unordered_map<uint64_t, stream_ptr> &stream_manage::get_all_streams() { return streams_; }

void stream_manage::add_stream(nng_stream *s) {
  SPDLOG_DEBUG("add_stream");
  auto temp_stream = std::make_shared<stream>(s,generate_stream_id(s));
  streams_.emplace(temp_stream->get_stream_id(), temp_stream);
}
void stream_manage::add_stream(stream_ptr s) {
  streams_.emplace(s->get_stream_id(), s);
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