// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-01.
//

#ifndef KUNGFU_IO_H
#define KUNGFU_IO_H

#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/nanomsg/socket.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>

namespace kungfu::yijinjing {
FORWARD_DECLARE_CLASS_PTR(session)

#define SETUP_TIMEOUT 50
#define TEST_USABLE_TIMEOUT 500
#define DEFAULT_RECV_TIMEOUT 100
#define DEFAULT_NOTICE_TIMEOUT 1000
#define REGISTER_TIMEOUT_SECONDS 60

class io_device : public resource {
public:
  io_device(data::location_ptr home, bool low_latency, bool lazy);

  ~io_device() override = default;

  bool is_usable() override { return publisher_ and observer_ and publisher_->is_usable() and observer_->is_usable(); }

  [[nodiscard]] bool is_lazy() const { return lazy_; }

  bool setup() override {
    bool prc = publisher_->setup();
    bool orc = observer_->setup();
    return prc && orc;
  }

  [[nodiscard]] const data::locator_ptr &get_locator() const { return home_->locator; }

  [[nodiscard]] const data::location_ptr &get_home() const { return home_; }

  [[nodiscard]] const data::location_ptr &get_live_home() const { return live_home_; }

  [[nodiscard]] bool is_low_latency() const { return low_latency_; }

  [[nodiscard]] bool is_resource_manager_required() const {
    return low_latency_ && lazy_ && home_->mode == kungfu::longfist::enums::mode::LIVE;
  }

  [[nodiscard]] const journal::bus_ptr &get_bus() const { return bus_; }

  journal::reader_ptr open_reader_to_subscribe();

  [[maybe_unused]] journal::reader_ptr open_reader(const data::location_ptr &location, uint32_t dest_id);

  journal::writer_ptr open_writer(uint32_t dest_id, uint64_t page_size = 0);

  journal::writer_ptr open_writer_at(const data::location_ptr &location, uint32_t dest_id, uint64_t page_size = 0);

  journal::writer_ptr open_hookable_writer(uint32_t dest_id, const journal::writer_hook_ptr &hook,
                                           uint64_t page_size = 0);

  [[maybe_unused]] journal::writer_ptr open_hookable_writer_at(const data::location_ptr &location, uint32_t dest_id,
                                                               const journal::writer_hook_ptr &hook,
                                                               uint64_t page_size = 0);

  [[nodiscard]] nanomsg::url_factory_ptr get_url_factory() const { return url_factory_; }

  [[nodiscard]] publisher_ptr get_publisher() { return publisher_; }

  [[nodiscard]] observer_ptr get_observer() { return observer_; }

  void set_begin_time(int64_t begin_time) { begin_time_ = begin_time; }

protected:
  data::location_ptr home_;
  data::location_ptr live_home_;
  const bool low_latency_;
  const bool lazy_;
  int64_t begin_time_;
  nanomsg::url_factory_ptr url_factory_;
  publisher_ptr publisher_;
  observer_ptr observer_;
  journal::bus_ptr bus_;
};

DECLARE_PTR(io_device)

class io_device_master : public io_device {
public:
  io_device_master(data::location_ptr home, bool low_latency);
};

DECLARE_PTR(io_device_master)

class io_device_client : public io_device {
public:
  io_device_client(data::location_ptr home, bool low_latency);

  bool is_usable() override;

  bool setup() override;
};

DECLARE_PTR(io_device_client)

class io_device_console : public io_device {
public:
  io_device_console(data::location_ptr home, int32_t console_width, int32_t console_height);

  [[maybe_unused]] void trace(int64_t begin_time, int64_t end_time, bool in, bool out, std::string csv);

  [[maybe_unused]] void show(int64_t begin_time, int64_t end_time, bool in, bool out, std::string csv);

private:
  int32_t console_width_;
  int32_t console_height_;
};

DECLARE_PTR(io_device_console)

class io_device_network : public resource {
public:
  io_device_network(bool is_low_latency) : is_low_latency_(is_low_latency) {
    stream_manager_ = std::make_shared<kungfu::yijinjing::webserver::stream_manage>();
  }

  ~io_device_network() override = default;

  bool is_usable() override { return true; }

  bool setup() override { return true; }

  kungfu::yijinjing::webserver::stream_manage_ptr get_stream_manager() { return stream_manager_; };

protected:
  kungfu::yijinjing::webserver::stream_manage_ptr stream_manager_;
  bool is_low_latency_;
};
DECLARE_PTR(io_device_network)

class io_device_network_server : public io_device_network {
public:
  io_device_network_server(const std::string &address, const std::vector<std::string> &paths,
                           bool is_low_latency = true, bool is_text_mode = true)
      : io_device_network(is_low_latency),
        http_server_(std::make_shared<kungfu::yijinjing::webserver::http_server>(address)) {
    for (const auto &path : paths) {
      http_server_->add_websocket(stream_manager_, path, is_text_mode);
    }
  };

  ~io_device_network_server() override = default;

private:
  kungfu::yijinjing::webserver::http_server_ptr http_server_;
};
DECLARE_PTR(io_device_network_server)

class io_device_network_client : public io_device_network {
public:
  io_device_network_client(bool is_low_latency = true) : io_device_network(is_low_latency){};

  ~io_device_network_client() override = default;

  // TODO: how to stand return connect failed?
  uint64_t connect_remote(const std::string &address) {
    auto client = std::make_shared<kungfu::yijinjing::webserver::webclient>(stream_manager_, address);
    webclients_.emplace_back(client);
    return client->get_stream_id();
  };

private:
  std::vector<kungfu::yijinjing::webserver::webclient_ptr> webclients_;
};
DECLARE_PTR(io_device_network_client)

void handle_sql_error(int rc, const std::string &error_tip);
void ensure_sqlite_initilize();
void ensure_sqlite_shutdown();
} // namespace kungfu::yijinjing
#endif // KUNGFU_IO_H
