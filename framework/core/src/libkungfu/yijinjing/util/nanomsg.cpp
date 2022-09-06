//
// Created by Keren Dong on 2019-05-25.
//

#include <kungfu/common.h>
#include <kungfu/yijinjing/nanomsg/socket.h>
#include <kungfu/yijinjing/util/util.h>

namespace kungfu::yijinjing::nanomsg {

const char *nn_exception::what() const throw() { return nng_strerror(errno_); }

int nn_exception::num() const { return errno_; }

socket::socket(protocol p, int buffer_size) : protocol_(p), buf_(buffer_size) {
  int rc;
  switch (p) {
  case protocol::REPLY:
    rc = nng_rep0_open(&sock_);
    break;
  case protocol::REQUEST:
    rc = nng_req0_open(&sock_);
    break;
  case protocol::PUSH:
    rc = nng_push0_open(&sock_);
    break;
  case protocol::PULL:
    rc = nng_pull0_open(&sock_);
    break;
  case protocol::PUBLISH:
    rc = nng_pub0_open(&sock_);
    break;
  case protocol::SUBSCRIBE:
    rc = nng_sub0_open(&sock_);
    break;
  default:
    SPDLOG_ERROR("unsupportted protocol {}", int(p));
  }

  if (rc < 0) {
    SPDLOG_DEBUG("can not create socket, {}: {}", int(p), nng_strerror(rc));
    throw nn_exception(rc);
  }
}

socket::~socket() { nng_close(sock_); }

void socket::setsockopt(const char *opt, const void *val, size_t valsz) {
  int rc = nng_socket_set(sock_, opt, val, valsz);
  if (rc != 0) {
    SPDLOG_DEBUG("can not setsockopt");
    throw nn_exception(rc);
  }
}

void socket::setsockopt_str(const char *opt, std::string value) { setsockopt(opt, value.c_str(), value.length()); }

void socket::setsockopt_int(const char *opt, int value) { setsockopt(opt, &value, sizeof(value)); }

void socket::getsockopt(const char *opt, void *val, size_t *valszp) {
  int rc = nng_socket_get(sock_, opt, val, valszp);
  if (rc != 0) {
    SPDLOG_DEBUG("can not getsockopt");
    throw nn_exception(rc);
  }
}

int socket::getsockopt_int(const char *opt) {
  int rc;
  size_t s = sizeof(rc);
  getsockopt(opt, &rc, &s);
  return rc;
}

int socket::listen(const std::string &path) {
  url_ = "ipc://" + path;
  int rc = nng_listener_create(&listener_, sock, url);
  if (rc < 0) {
    SPDLOG_ERROR("can not listen to {}", url_);
    throw nn_exception(rc);
  }

  return rc
}

int socket::dial(const std::string &path) {
  // url_ = "ipc://" + path;
  // int rc = nn_connect(sock_, url_.c_str());
  // if (rc < 0) {
  //   SPDLOG_ERROR("can not connect to {}", url_);
  //   throw nn_exception();
  // }
  // return rc;
  return 0;
}

void socket::close() {
  int rc = nng_close(sock_);
  if (rc != 0) {
    SPDLOG_DEBUG("can not close");
    throw nn_exception(rc);
  }
}

int socket::send(const std::string &msg, int flags) const {
  // int rc = nn_send(sock_, msg.c_str(), msg.length(), flags);
  // if (rc < 0) {
  //   if (nn_errno() != EAGAIN) {
  //     SPDLOG_ERROR("can not send to {} errno [{}] {}", url_, nn_errno(), nn_strerror(nn_errno()));
  //     throw nn_exception();
  //   }
  //   return -1;
  // }
  // return rc;
  return 0;
}

int socket::recv(int flags) {
  // int rc = nn_recv(sock_, buf_.data(), buf_.size(), flags);
  // if (rc < 0) {
  //   switch (nn_errno()) {
  //   case ETIMEDOUT:
  //   case EAGAIN:
  //     break;
  //   case EINTR: {
  //     SPDLOG_WARN("interrupted when receiving from [{}]", url_);
  //     break;
  //   }
  //   default: {
  //     SPDLOG_ERROR("can not recv from [{}] errno [{}] {}", url_, nn_errno(), nn_strerror(nn_errno()));
  //     throw nn_exception();
  //   }
  //   }
  //   message_.assign(buf_.data(), 0);
  //   return 0;
  // } else {
  //   message_.assign(buf_.data(), rc);
  //   return rc;
  // }
  return 0;
}

const std::string &socket::recv_msg(int flags) {
  // recv(flags);
  // return message_;
  return "";
}

int socket::send_json(const nlohmann::json &msg, int flags) const { return send(msg.dump(), flags); }

nlohmann::json socket::recv_json(int flags) {
  //   int rc = 0;
  //   if ((rc = recv(flags)) > 0) {
  //     SPDLOG_INFO("parsing json {} {}", rc, message_);
  //     return nlohmann::json::parse(message_);
  //   } else {
  //     return nlohmann::json();
  //   }
  return nlohmann::json();
}

const std::string &socket::request(const std::string &json_message) {
  // send(json_message);
  // return recv_msg();
  return "";
}
} // namespace kungfu::yijinjing::nanomsg
