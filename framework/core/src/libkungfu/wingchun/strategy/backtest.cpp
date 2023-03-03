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

namespace kungfu::wingchun::strategy {

BacktestContext::BacktestContext(apprentice &app, const rx::connectable_observable<event_ptr> &events,
                                 Matcher_ptr matcher)
    : Context(app, events), broker_client_(app_), bookkeeper_(app_, broker_client_), matcher_(std::move(matcher)) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void BacktestContext::on_start() {
  auto writer = app_.get_writer(location::PUBLIC);
  writer->mark_at(app_.get_begin_time(), app_.get_begin_time(), RequestStart::tag);
  // broker_client_.on_start(events_);
  bookkeeper_.on_start(events_);
  events_ | is_own<Quote>(get_broker_client()) | $$(matcher_->on_quote(event->data<Quote>()));
  events_ | is_own<Entrust>(get_broker_client()) | $$(matcher_->on_entrust(event->data<Entrust>()));
  events_ | is_own<Transaction>(get_broker_client()) | $$(matcher_->on_transaction(event->data<Transaction>()));
  events_ | is(OrderInput::tag) | $$(matcher_->on_order_input(event->data<OrderInput>()));
  events_ | is(OrderAction::tag) | $$(matcher_->on_order_action(event->data<OrderAction>()));
}

bool BacktestContext::is_started() const { return true; }

void BacktestContext::prepare(const event_ptr &event) {}

int64_t BacktestContext::now() const { return app_.now(); }

void BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  //   app_.add_timer(nanotime, callback);
}

void BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  //   app_.add_time_interval(duration, callback);
}

void BacktestContext::add_account(const std::string &source, const std::string &account) {}

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

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {
  // not implemented
}

uint64_t BacktestContext::insert_block_message(const std::string &source, const std::string &account,
                                               uint32_t opponent_seat, uint64_t match_number, bool is_specific) {
  // not implemented
  return {};
}

uint64_t BacktestContext::insert_order(const std::string &instrument_id, const std::string &exchange_id,
                                       const std::string &source, const std::string &account, double limit_price,
                                       int64_t volume, PriceType type, Side side, Offset offset, HedgeFlag hedge_flag,
                                       bool is_swap, uint64_t block_id, uint64_t parent_id) {
  // auto account_location_uid = get_td_location_uid(source, account);
  auto insert_time = now();
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }
  auto writer = app_.get_writer(location::PUBLIC);
  OrderInput &input = writer->open_data<OrderInput>(now());
  input.order_id = writer->current_frame_uid();
  strcpy(input.instrument_id, instrument_id.c_str());
  strcpy(input.exchange_id, exchange_id.c_str());
  input.instrument_type = instrument_type;
  input.limit_price = limit_price;
  input.frozen_price = limit_price;
  input.volume = volume;
  input.price_type = type;
  input.side = side;
  input.offset = offset;
  input.hedge_flag = hedge_flag;
  input.block_id = block_id;
  input.is_swap = is_swap;
  input.insert_time = insert_time;
  writer->close_data(now());
  // TODO
  // bookkeeper_.on_order_input(app_.now(), app_.get_home_uid(), account_location_uid, input);
  return input.order_id;
}

std::vector<uint64_t> BacktestContext::insert_batch_orders(
    const std::string &source, const std::string &account, const std::vector<std::string> &instrument_ids,
    const std::vector<std::string> &exchange_ids, std::vector<double> limit_prices, std::vector<int64_t> volumes,
    std::vector<longfist::enums::PriceType> types, std::vector<longfist::enums::Side> sides,
    std::vector<longfist::enums::Offset> offsets, std::vector<longfist::enums::HedgeFlag> hedge_flags,
    std::vector<bool> is_swaps) {
  std::vector<uint64_t> order_ids{};
  bool flag = instrument_ids.size() == exchange_ids.size() and //
              instrument_ids.size() == limit_prices.size() and //
              instrument_ids.size() == volumes.size() and      //
              instrument_ids.size() == types.size() and        //
              instrument_ids.size() == sides.size() and        //
              instrument_ids.size() == offsets.size() and      //
              instrument_ids.size() == hedge_flags.size() and  //
              instrument_ids.size() == is_swaps.size();
  if (not flag) {
    SPDLOG_ERROR("Batch size not equals!");
    return order_ids;
  }
  for (int i = 0; i < instrument_ids.size(); ++i) {
    uint64_t order_id =
        insert_order(instrument_ids.at(i), exchange_ids.at(i), source, account, limit_prices.at(i), volumes.at(i),
                     types.at(i), sides.at(i), offsets.at(i), hedge_flags.at(i), is_swaps.at(i));
    order_ids.push_back(order_id);
  }
  return order_ids;
}

std::vector<uint64_t> BacktestContext::insert_array_orders(const std::string &source, const std::string &account,
                                                           std::vector<longfist::types::OrderInput> order_inputs) {
  std::vector<uint64_t> order_ids{};
  for (const OrderInput &input : order_inputs) {
    uint64_t order_id =
        insert_order(input.instrument_id, input.exchange_id, source, account, input.limit_price, input.volume,
                     input.price_type, input.side, input.offset, input.hedge_flag, input.is_swap);
    order_ids.push_back(order_id);
  }
  return order_ids;
}

uint64_t BacktestContext::insert_basket_order(uint64_t basket_id, const std::string &source, const std::string account,
                                              longfist::enums::Side side, longfist::enums::PriceType price_type,
                                              longfist::enums::PriceLevel price_level, double price_offset,
                                              int64_t volume) {
  return {};
}

uint64_t BacktestContext::cancel_order(uint64_t order_id) {
  auto writer = app_.get_writer(location::PUBLIC);
  OrderAction &action = writer->open_data<OrderAction>(now());

  action.order_action_id = writer->current_frame_uid();
  action.order_id = order_id;
  action.action_flag = OrderActionFlag::Cancel;

  writer->close_data(now());
  return action.order_action_id;
}

int64_t BacktestContext::get_trading_day() const { return time::calendar_day_start(now()); }

broker::Client &BacktestContext::get_broker_client() { return broker_client_; }

book::Bookkeeper &BacktestContext::get_bookkeeper() { return bookkeeper_; }

const location_ptr BacktestContext::find_md_location(const std::string &source) {
  uint32_t cache_uid = hash_backtest_cache(source, app_.get_begin_time(), app_.get_end_time());
  auto cache_location =
      location::make_shared(mode::BACKTEST, category::MD, source, fmt::format("{:08x}", cache_uid), app_.get_locator());
  return cache_location;
}

void BacktestContext::req_history_order(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_history_trade(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_deregister() { app_.request_deregister(); }

void BacktestContext::update_strategy_state(StrategyStateUpdate &state_update) {
  // not implemented
}

} // namespace kungfu::wingchun::strategy
