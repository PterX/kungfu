// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-01.
//

#include <kungfu/common.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <kungfu/yijinjing/util/os.h>
#include <utility>

using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace std::chrono;

namespace kungfu::yijinjing::practice {

apprentice::apprentice(location_ptr home, bool low_latency, std::string arguments)
    : hero(std::make_shared<io_device_client>(home, low_latency)), trading_day_(time::today_start()), cleaner_(*this),
      arguments_(std::move(arguments)) {}

bool apprentice::is_started() const { return started_; }

void apprentice::pause() { started_ = false; }

uint32_t apprentice::get_master_command_uid() const { return master_cmd_location_->uid; }

int64_t apprentice::get_checkin_time() const { return checkin_time_; }

int64_t apprentice::get_last_active_time() const { return last_active_time_; }

int64_t apprentice::get_trading_day() const { return trading_day_; }

const cache::bank &apprentice::get_state_bank() const { return state_bank_; }

void apprentice::request_read_from(int64_t trigger_time, uint32_t source_id, int64_t from_time) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    require_read_from(trigger_time, get_master_command_uid(), source_id, from_time);
  }
}

void apprentice::request_read_from_public(int64_t trigger_time, uint32_t source_id, int64_t from_time) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    require_read_from_public(trigger_time, get_master_command_uid(), source_id, from_time);
  }
}

void apprentice::request_read_from_sync(int64_t trigger_time, uint32_t source_id, int64_t from_time) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    require_read_from_sync(trigger_time, get_master_command_uid(), source_id, from_time);
  }
}

void apprentice::request_read_from_source_to_dest(int64_t trigger_time, const location_ptr &source_location,
                                                  uint32_t dest_id) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    reader_join(source_location->uid, dest_id, trigger_time);
  }
}

void apprentice::request_write_to(int64_t trigger_time, uint32_t dest_id) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    require_write_to(trigger_time, get_master_command_uid(), dest_id);
  }
}

void apprentice::request_write_to_band(int64_t trigger_time, const location_ptr &location) {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    require_write_to_band(trigger_time, get_master_command_uid(), location);
  }
}

uint32_t apprentice::request_band(const std::string &band_name) {
  auto io_device = get_io_device();
  auto home = io_device->get_home();
  auto band_location = location::make_shared(home->mode, home->category, home->group, band_name, get_locator());
  request_write_to_band(now(), band_location);
  return band_location->uid;
}

void apprentice::request_cached_reader_writer() {
  if (get_io_device()->get_home()->mode == mode::LIVE) {
    if (writers_.find(get_master_command_uid()) == writers_.end()) {
      SPDLOG_ERROR("no writer for {}", get_location_uname(get_master_command_uid()));
      return;
    }

    if (get_live_home_uid() != cached_home_location_->uid) {
      if (registry_.find(cached_home_location_->uid) == registry_.end()) {
        SPDLOG_ERROR("no register in registry_ {}", get_location_uname(get_master_command_uid()));
        return;
      }

      request_write_to(now(), cached_home_location_->uid);
      request_read_from(now(), cached_home_location_->uid, now());

    } else {
      // At cached case, pass the restore, start directly
      auto writer = get_writer(get_master_command_uid());
      RequestCachedDone &rcd = writer->open_data<RequestCachedDone>();
      rcd.dest_id = get_io_device()->get_live_home()->uid;
      writer->close_data();
    }
  }
}

void apprentice::request_cached(uint32_t source_id) {
  auto writer = get_writer(source_id);
  writer->mark(now(), RequestCached::tag);
}

void apprentice::add_timer(int64_t nanotime, const std::function<void(const event_ptr &)> &callback) {
  events_ | timer(nanotime) | $([&, callback](const event_ptr &event) { callback(event); });
}

void apprentice::add_time_interval(int64_t duration, const std::function<void(const event_ptr &)> &callback) {
  events_ | time_interval(std::chrono::nanoseconds(duration)) |
      $([&, callback](const event_ptr &event) { callback(event); });
}

void apprentice::on_trading_day(const event_ptr &event, int64_t daytime) {}

bool apprentice::release_page() {
  bool result = false;
  result |= reader_->release_page();
  for (auto &iter : writers_) {
    result |= iter.second->release_page();
  }
  return result;
}

void apprentice::react() {
  events_ | is(TimeReset::tag) | first() | $$(reset_time(event->data<TimeReset>()));
  events_ | is(Location::tag) | $$(add_location(event->gen_time(), event->data<Location>()));
  events_ | is(Register::tag) | $$(on_register(event->trigger_time(), event->data<Register>()));
  events_ | is(RequestReadFromOthers::tag) | $$(on_request_read_from_others(event));
  events_ | is(Deregister::tag) | $$(on_deregister(event));
  events_ | is(RequestReadFrom::tag) | $$(on_read_from(event));
  events_ | is(CachedReadyToRead::tag) | $$(on_cached_ready_to_read());
  events_ | is(RequestReadFromPublic::tag) | $$(on_read_from_public(event));
  events_ | is(RequestReadFromSync::tag) | $$(on_read_from_sync(event));
  events_ | is(RequestWriteTo::tag) | $$(on_write_to(event));
  events_ | is(RequestWriteToBand::tag) | $$(on_write_to_band(event));
  events_ | is(Channel::tag) | $$(register_channel(event->gen_time(), event->data<Channel>()));
  events_ | is(Band::tag) | $$(register_band(event->gen_time(), event->data<Band>()));
  events_ | is(TradingDay::tag) | $$(on_trading_day(event, event->data<TradingDay>().timestamp));
  events_ | is(RequestStop::tag) | to(get_home_uid()) | $$(signal_stop());
  events_ | take_until(events_ | is(RequestStart::tag)) | $$(feed_state_data(event, state_bank_));

  SPDLOG_TRACE("building reactive event handlers");
  on_react();
  cleaner_.on_react();

  if (get_io_device()->get_home()->mode == mode::LIVE) {
    auto self_register_event = events_ | skip_until(events_ | is(Register::tag) | filter([&](const event_ptr &event) {
                                                      auto uid = event->data<Register>().location_uid;
                                                      return uid == get_home_uid();
                                                    })) |
                               first();

    self_register_event | rx::timeout(seconds(60), observe_on_new_thread()) |
        $(
            [&](const event_ptr &event) {
              // this subscriber will quit when register is done, no worry for performance.
            },
            [&](std::exception_ptr e) {
              try {
                std::rethrow_exception(e);
              } catch (const timeout_error &ex) {
                SPDLOG_ERROR("app register timeout");
                hero::signal_stop();
              }
            });

    self_register_event | $([&](const event_ptr &event) {
      auto data = event->data<Register>();
      last_active_time_ = data.last_active_time;
      checkin_time_ = data.checkin_time;
      reader_->join(master_cmd_location_, get_live_home_uid(), begin_time_);
    });

    auto cached_register_event = events_ | is(Register::tag) | filter([&](const event_ptr &event) {
                                   auto register_data = event->data<Register>();
                                   return register_data.location_uid == cached_home_location_->uid;
                                 }) |
                                 filter([&](const event_ptr &event) {
                                   if (writers_.find(get_master_command_uid()) != writers_.end()) {
                                     return true;
                                   }
                                   return false;
                                 }) |
                                 first();
    cached_register_event | $$(request_cached_reader_writer());

    expect_start();
    checkin();
  }
  if (get_io_device()->get_home()->mode == mode::REPLAY) {
    reader_->join(master_cmd_location_, get_live_home_uid(), begin_time_);
    expect_start();
  }
  if (get_io_device()->get_home()->mode == mode::BACKTEST) {
    // dest_id 0 should be configurable TODO
    auto home = get_io_device()->get_home();
    auto bt_location = location::make_shared(mode::BACKTEST, category::MD, home->group, home->name, get_locator());
    reader_->join(bt_location, location::PUBLIC, begin_time_);
    started_ = true;
    on_start();
  }
}

void apprentice::on_active() {}

void apprentice::on_frame() {
  for (const uint32_t dest_id : try_write_dest_ids_) {
    request_write_to(now(), dest_id);
  }
  try_write_dest_ids_.clear();
}

void apprentice::on_react() {}

void apprentice::on_start() {}

void apprentice::on_request_read_from_others(const event_ptr &event) {
  const auto &request = event->data<RequestReadFromOthers>();
  if (has_location(request.source_id)) {
    reader_->join(get_location(request.source_id), request.dest_id, request.from_time);
  }
}

void apprentice::on_register(int64_t trigger_time, const Register &register_data) {
  register_location(trigger_time, register_data);
}

void apprentice::on_deregister(const event_ptr &event) {
  uint32_t location_uid = event->data<Deregister>().location_uid;
  if (location_uid == get_live_home_uid()) {
    return;
  }

  reader_->disjoin(location_uid);
  deregister_channel(location_uid);
  deregister_band(location_uid);
  deregister_location(event->trigger_time(), location_uid);
}

void apprentice::on_read_from(const event_ptr &event) { do_read_from<RequestReadFrom>(event, get_live_home_uid()); }

void apprentice::on_read_from_public(const event_ptr &event) { do_read_from<RequestReadFromPublic>(event, 0); }

void apprentice::on_read_from_sync(const event_ptr &event) { do_read_from<RequestReadFromSync>(event, location::SYNC); }

void apprentice::on_write_to(const event_ptr &event) {
  auto dest_id = event->data<RequestWriteTo>().dest_id;
  if (writers_.find(dest_id) == writers_.end()) {
    writers_.emplace(dest_id, get_io_device()->open_writer(dest_id));
    if (dest_id == get_master_command_uid()) {
      master_cmd_writer_for_thread_ = get_writer(dest_id);
    }
  }
}

void apprentice::on_write_to_band(const event_ptr &event) {
  auto dest_id = event->data<RequestWriteToBand>().location_uid;
  if (writers_.find(dest_id) == writers_.end()) {
    writers_.emplace(dest_id, get_io_device()->open_writer(dest_id));
  }
}

void apprentice::on_cached_ready_to_read() { request_cached(cached_home_location_->uid); }

[[maybe_unused]] int apprentice::get_observer_recv_timeout() const {
  return get_io_device()->get_observer()->get_recv_timeout();
}

void apprentice::reader_join(uint32_t source_id, uint32_t dest_id, int64_t from_time) {

  if (not has_location(source_id)) {
    SPDLOG_ERROR("no location {}", source_id);
    return;
  }

  reader_->join(get_location(source_id), dest_id, from_time);

  if (not has_writer(get_master_command_uid())) {
    SPDLOG_ERROR("no master cmd writer");
    return;
  }

  auto writer = get_writer(get_master_command_uid());
  auto &request = writer->open_data<RequestReadFromOthers>(now());
  request.source_id = source_id;
  request.dest_id = dest_id;
  request.from_time = from_time;
  writer->close_data();
}

void apprentice::checkin() {
  auto now = time::now_in_nano();
  auto home = get_home();
  Register register_data{};
  register_data.mode = home->mode;
  register_data.category = home->category;
  register_data.group = home->group;
  register_data.name = home->name;
  register_data.location_uid = home->uid;
  register_data.pid = GETPID();
  register_data.checkin_time = now;
  register_data.last_active_time = now;

  SPDLOG_INFO("app checkin");

  while (not is_usable()) {
    SPDLOG_WARN("publisher is not usable");
  }

  SPDLOG_INFO("io is usable");
  get_io_device()->get_publisher()->publish(make_nano_msg(get_home_uid(), master_home_location_->uid, register_data),
                                            0);
}

void apprentice::expect_start() {
  reader_->join(master_home_location_, location::PUBLIC, begin_time_);
  events_ | is(RequestStart::tag) | first() | $([&](const event_ptr &event) {
    started_ = true;
    SPDLOG_INFO("ready to start");
    on_start();
  });
}

void apprentice::reset_time(const longfist::types::TimeReset &time_reset) {
  time::reset(time_reset.system_clock_count, time_reset.steady_clock_count);
}

yijinjing::journal::writer_ptr &apprentice::get_thread_writer() {
  if (not thread_writer_) {
    uint32_t dest_id = kungfu::yijinjing::util::get_thread_id();
    thread_writer_ = get_io_device()->open_writer(dest_id);

    /// join channel in sub-thread will crash, so tell master to ask myself to join
    /// do not use writer because of multi-thread concurrency issues
    if (not master_cmd_writer_for_thread_) {
      SPDLOG_ERROR("has no writer of master_cmd: {:8x}", get_master_command_uid());
    }
    RequestReadFromOthers &request = master_cmd_writer_for_thread_->open_data<RequestReadFromOthers>();
    request.source_id = get_home_uid();
    request.dest_id = dest_id;
    request.from_time = now();
    SPDLOG_TRACE("RequestReadFromOthers: {}", request.to_string());
    master_cmd_writer_for_thread_->close_data();
  }
  return thread_writer_;
}

} // namespace kungfu::yijinjing::practice
