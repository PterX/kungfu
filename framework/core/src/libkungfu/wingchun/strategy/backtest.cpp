// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/7/20.
//

#include <fmt/format.h>

#include <kungfu/wingchun/strategy/backtest.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;
namespace fs = std::filesystem;

namespace kungfu::wingchun::strategy {

BacktestContext::BacktestContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app_), bookkeeper_(app_, broker_client_)
{
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
  std::string journal_dir =  app_.get_locator()->layout_dir(app_.get_home(), layout::JOURNAL);
  fs::remove_all(journal_dir);
  app_.get_writers().emplace(location::PUBLIC, app_.get_io_device()->open_writer(location::PUBLIC));
  app_.get_reader()->join(app_.get_home(), location::PUBLIC, app_.get_begin_time());
}

void BacktestContext::on_start() {
    app_.get_reader()->join(app_.get_home(), location::PUBLIC, app_.get_begin_time());
    auto writer = app_.get_writer(location::PUBLIC);
    writer->mark_at(app_.get_begin_time(), app_.get_begin_time(), RequestStart::tag);
    // broker_client_.on_start(events_);
    bookkeeper_.on_start(events_);

}

bool BacktestContext::is_started() const { return true; }

void BacktestContext::prepare(const event_ptr &event) {
}

int64_t BacktestContext::now() const { return app_.now(); }

void BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  //   app_.add_timer(nanotime, callback);
}

void BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  //   app_.add_time_interval(duration, callback);
}

void BacktestContext::add_account(const std::string &source, const std::string &account) {
  //   uint32_t hashed_account = hash_account(source, account);

  //   if (td_locations_.find(hashed_account) != td_locations_.end()) {
  //     throw wingchun_error(fmt::format("duplicated account {}_{}", source, account));
  //   }

  //   auto home = app_.get_io_device()->get_home();
  //   auto account_location = location::make_shared(mode::LIVE, category::TD, source, account, home->locator);
  //   if (home->mode == mode::LIVE and not app_.has_location(account_location->uid)) {
  //     throw wingchun_error(fmt::format("invalid account {}_{}", source, account));
  //   }

  //   td_locations_.emplace(hashed_account, account_location);
  //   td_locations_.emplace(account_location->uid, account_location);
  //   account_location_ids_.emplace(hashed_account, account_location->uid);

    // broker_client_.enroll_account(account_location);
}

void BacktestContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                const std::string &exchange_ids) {
    auto md_location = find_md_location(source);
    add_location(app_, md_location);
    app_.get_reader()->join(md_location, location::PUBLIC, app_.get_begin_time());
    for (const auto &instrument_id : instrument_ids) {
      broker_client_.subscribe(md_location, exchange_ids, instrument_id);
    }
}

void BacktestContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                    uint64_t data_type) {
    broker_client_.subscribe_all(find_md_location(source), market_type, instrument_type, data_type);
}

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {}

uint64_t BacktestContext::insert_block_message(const std::string &source, const std::string &account,
                                               uint32_t opponent_seat, uint64_t match_number, bool is_specific) {
  return {};
}

uint64_t BacktestContext::insert_order(const std::string &instrument_id, const std::string &exchange_id,
                                       const std::string &source, const std::string &account, double limit_price,
                                       int64_t volume, PriceType type, Side side, Offset offset, HedgeFlag hedge_flag,
                                       bool is_swap, uint64_t block_id) {
  return {};
}

std::vector<uint64_t> BacktestContext::insert_batch_orders(
    const std::string &source, const std::string &account, const std::vector<std::string> &instrument_ids,
    const std::vector<std::string> &exchange_ids, std::vector<double> limit_prices, std::vector<int64_t> volumes,
    std::vector<longfist::enums::PriceType> types, std::vector<longfist::enums::Side> sides,
    std::vector<longfist::enums::Offset> offsets, std::vector<longfist::enums::HedgeFlag> hedge_flags,
    std::vector<bool> is_swaps) {
  return {};
}

std::vector<uint64_t> BacktestContext::insert_array_orders(const std::string &source, const std::string &account,
                                                           std::vector<longfist::types::OrderInput> order_inputs) {
  return {};
}

uint64_t BacktestContext::cancel_order(uint64_t order_id) { return {}; }

// const location_map &BacktestContext::list_md() const { return md_locations_; }

// const location_map &BacktestContext::list_op() const { return op_locations_; }

// const location_map &BacktestContext::list_accounts() const { return td_locations_; }

int64_t BacktestContext::get_trading_day() const { return app_.get_trading_day(); }

broker::Client &BacktestContext::get_broker_client() {
   return broker_client_; 
  // return {};
   }

book::Bookkeeper &BacktestContext::get_bookkeeper() { 
  return bookkeeper_; 
  }

const location_ptr BacktestContext::find_md_location(const std::string &source) {
  uint32_t cache_uid = hash_backtest_cache(source, app_.get_begin_time(), app_.get_end_time());
  auto cache_location =
      location::make_shared(mode::BACKTEST, category::MD, source, fmt::format("{:08x}", cache_uid), app_.get_locator());
  return cache_location;
}

void BacktestContext::req_history_order(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_history_trade(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_deregister() {
  app_.request_deregister();
}

void BacktestContext::update_strategy_state(StrategyStateUpdate &state_update) {
  //   auto writer = app_.get_writer(location::PUBLIC);
  //   state_update.update_time = now();
  //   writer->write(state_update.update_time, state_update);
}

} // namespace kungfu::wingchun::strategy
