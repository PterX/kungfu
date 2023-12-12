// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/7/20.
//

#include <fmt/format.h>
#include <kungfu/wingchun/strategy/backtest.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/nanomsg/socket.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun::tool;
using kungfu::yijinjing::nanomsg::nanomsg_json;

namespace kungfu::wingchun::strategy {

BacktestContext::BacktestContext(practice::apprentice &app, const rx::connectable_observable<event_ptr> &events,
                                 Matcher_ptr matcher, SliceIndexer_ptr from_indexer, SliceIndexer_ptr to_indexer,
                                 Report_ptr report, int64_t time_interval, std::string backtest_config)
    : Context(app, events), broker_client_(app_), bookkeeper_(app_, broker_client_), matcher_(std::move(matcher)),
      from_indexer_(from_indexer),
      slice_tool_(std::make_shared<SliceTool>(category::STRATEGY, app.get_home()->group, app.get_home()->name,
                                              std::move(to_indexer))),
      report_(std::move(report)), time_interval_(time_interval), backtest_config_(std::move(backtest_config)) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void BacktestContext::on_start() {
  if (not is_bypass_accounting()) {
    bookkeeper_.on_start(events_);
  }
  events_ | is_own<Quote>(get_broker_client()) |
      $$(matcher_->on_quote(event->data<Quote>()); report_->on_quote(event->data<Quote>()););
  events_ | is_own<Entrust>(get_broker_client()) |
      $$(matcher_->on_entrust(event->data<Entrust>()); report_->on_entrust(event->data<Entrust>()););
  events_ | is_own<Transaction>(get_broker_client()) |
      $$(matcher_->on_transaction(event->data<Transaction>()); report_->on_transaction(event->data<Transaction>()););
  events_ | is_own<Tree>(get_broker_client()) |
      $$(matcher_->on_tree(event->data<Tree>()); report_->on_tree(event->data<Tree>()););
  events_ | is(SyntheticData::tag) | $$(report_->on_read_synthetic_data(event->data<SyntheticData>()));
  events_ | is(OrderInput::tag) |
      $$(const auto &order_input = event->data<OrderInput>();
         add_order_id(*matcher_, order_input.order_id, event->source(), event->dest());
         matcher_->on_order_input(order_input));
  events_ | is(Order::tag) |
      $$(const auto &order = event->data<Order>(); report_->on_order(order);
         if (is_final_status(order.status)) { remove_order_id(*matcher_, order.order_id); });
  events_ | is(Trade::tag) | $$(report_->on_trade(event->data<Trade>()));
  events_ | is(OrderAction::tag) |
      $$(const auto &order_action = event->data<OrderAction>();
         add_order_id(*matcher_, order_action.order_id, event->source(), event->dest());
         matcher_->on_order_action(order_action));
  events_ | is(OrderActionError::tag) | $$(remove_order_id(*matcher_, event->data<OrderActionError>().order_id));
  events_ | $$(on_timer_check(); lease_expired_check(););
  init_time_events();
}

bool BacktestContext::is_started() const { return true; }

void BacktestContext::prepare(const event_ptr &event) {}

int64_t BacktestContext::now() const { return app_.now(); }

int32_t BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  const int32_t timer_id = timer_usage_count_++;
  if (timer_id < 0) {
    throw wingchun_error(fmt::format("timer_id={} is overflow", timer_id));
  }
  pre_timer_callbacks_.emplace(nanotime, TimerTask{timer_id, callback});
  return timer_id;
}

int32_t BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  const int32_t timer_id = timer_usage_count_++;
  return add_timer_interval_helper(duration, timer_id, callback);
}

int32_t BacktestContext::add_timer_interval_helper(int64_t duration, int32_t timer_id,
                                                   const std::function<void(event_ptr)> &callback) {
  auto timer_callback = [this, callback, duration, timer_id](event_ptr event) {
    callback(event);
    this->add_timer_interval_helper(duration, timer_id, callback);
  };
  pre_timer_callbacks_.emplace(now() + duration, TimerTask{timer_id, timer_callback});
  return timer_id;
}

void BacktestContext::clear_timer(int32_t timer_id) {
  if (timer_id <= protected_timer_id_) {
    SPDLOG_WARN("timer_id={} lower than {} is reserved which is not allowed to clear", timer_id, protected_timer_id_);
    return;
  }
  std::erase_if(pre_timer_callbacks_,
                [timer_id](const auto &timer_task) { return timer_task.second.timer_id == timer_id; });
  std::erase_if(timer_callbacks_,
                [timer_id](const auto &timer_task) { return timer_task.second.timer_id == timer_id; });
}

void BacktestContext::on_timer_check() {
  timer_callbacks_.merge(pre_timer_callbacks_);
  auto now_time = now();
  for (auto it = timer_callbacks_.begin(); it != timer_callbacks_.end();) {
    if (it->first <= now_time) {
      nlohmann::json time_event;
      time_event["msg_type"] = Time::tag;
      time_event["gen_time"] = now_time;
      time_event["trigger_time"] = now_time;
      time_event["source"] = app_.get_live_home_uid();
      time_event["dest"] = app_.get_live_home_uid();
      time_event["data"] = nlohmann::json::object();
      // TODO use app_.make_nano_msg instead
      // Time time_event{};
      it->second.call_back(std::make_shared<nanomsg_json>(time_event.dump()));
      it = timer_callbacks_.erase(it);
    } else {
      return;
    }
  }
}

void BacktestContext::add_account(const std::string &source, const std::string &account) {
  auto td_location = find_td_location(source, account, false);
  add_location(app_, td_location);
}

void BacktestContext::lease_expired_check() {
  int64_t now_time = now();
  for (auto it = lease_locations_.begin(); it != lease_locations_.end();) {
    if (it->first < now_time) {
      for (const auto &expired_location : it->second) {
        SPDLOG_TRACE("sliced location expired, locator={}, location={} disjoining.",
                     expired_location->locator->get_root(), expired_location->uname);
        app_.get_reader()->disjoin(expired_location, location::PUBLIC);
      }
      it = lease_locations_.erase(it);
    } else {
      break;
    }
  }
}

void BacktestContext::init_time_events() {
  auto writer = app_.get_writer(app_.get_home_uid());
  nlohmann::json j_obj = nlohmann::json::parse(backtest_config_);
  parse_then_write_in_timer<Commission>(j_obj, writer);
  parse_then_write_in_timer<Instrument>(j_obj, writer);

  auto write_next_time_mark = [writer, this](auto e) {
    auto next_time = now() + time_interval_;
    writer->mark_at(next_time, next_time, Time::tag);
  };
  write_next_time_mark(nullptr);
  protected_timer_id_ = add_time_interval(time_interval_, write_next_time_mark);

  SPDLOG_DEBUG("init {} Time events done.", (app_.get_end_time() - app_.get_begin_time()) / time_interval_);
}

void BacktestContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                const std::string &exchange_id) {
  for (auto data_type : {Quote::tag, Tree::tag, Entrust::tag, Transaction::tag}) {
    for (const auto &instrument_id : instrument_ids) {
      int64_t slice_begin_time = app_.now();
      int64_t slice_end_time{INT64_MAX};
      do {
        auto md_location = from_indexer_->find_md_slice_location(slice_begin_time, source, source, instrument_id,
                                                                 exchange_id, data_type);
        slice_end_time = from_indexer_->get_md_slice_end_time(slice_begin_time, source, source, instrument_id,
                                                              exchange_id, data_type);
        if (md_location->locator->list_page_id(md_location, location::PUBLIC).empty()) {
          SPDLOG_WARN("md public journal in locator={}, location={} not exists", md_location->locator->get_root(),
                      md_location->uname);
        }

        add_location(app_, md_location);
        for (const auto dest_id : md_location->locator->list_location_dest(md_location)) {
          SPDLOG_TRACE("subscribed md dest {}, locator={}, location={}", dest_id, md_location->locator->get_root(),
                       md_location->uname);
          app_.get_reader()->join(md_location, dest_id, slice_begin_time);
        }

        broker_client_.subscribe(md_location, exchange_id, instrument_id);
        lease_locations_[slice_end_time].push_back(std::move(md_location));
      } while ((slice_begin_time = 1 + slice_end_time) < app_.get_end_time());
    }
  }
}

void BacktestContext::unsubscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                  const std::string &exchange_id) {
  std::for_each(instrument_ids.begin(), instrument_ids.end(), [&exchange_id, this](const auto &instrument_id) {
    broker_client_.unsubscribe(exchange_id, instrument_id);
  });
}

void BacktestContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                    uint64_t data_type) {
  throw wingchun_error(fmt::format("not support subscribe_all in backtest mode"));
}

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {
  int64_t slice_begin_time = app_.now();
  int64_t slice_end_time{INT64_MAX};
  do {
    auto op_location = from_indexer_->find_operator_slice_location(slice_begin_time, group, name);
    slice_end_time = from_indexer_->get_operator_slice_end_time(slice_begin_time, group, name);
    if (op_location->locator->list_page_id(op_location, location::PUBLIC).empty()) {
      SPDLOG_WARN("operator public journal in locator={}, location={} not exists", op_location->locator->get_root(),
                  op_location->uname);
      continue;
    }

    add_location(app_, op_location);
    for (const auto dest_id : op_location->locator->list_location_dest(op_location)) {
      SPDLOG_TRACE("subscribed operator dest {}, locator={}, location={}", dest_id, op_location->locator->get_root(),
                   op_location->uname);
      app_.get_reader()->join(op_location, dest_id, slice_begin_time);
    }

    broker_client_.enroll_operator(op_location);
    lease_locations_[slice_end_time].push_back(std::move(op_location));
  } while ((slice_begin_time = 1 + slice_end_time) < app_.get_end_time());
}

uint64_t BacktestContext::insert_block_message(const std::string &source, const std::string &account,
                                               const std::string &opponent_seat, uint64_t match_number,
                                               bool is_specific) {
  // not implemented
  return {};
}

uint64_t BacktestContext::insert_order(const std::string &instrument_id, const std::string &exchange_id,
                                       const std::string &source, const std::string &account, double limit_price,
                                       int64_t volume, PriceType type, Side side, Offset offset, const std::string &contract_id, HedgeFlag hedge_flag,
                                       bool is_swap, uint64_t block_id, uint64_t parent_id) {
  auto insert_time = now();
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }
  auto &&writer = app_.get_writer(location::PUBLIC);
  uint32_t td_dest = find_td_location(source, account)->uid;
  OrderInput input{};
  input.order_id = get_order_id(writer, td_dest);
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
  writer->write_raw_at_as(now(), now(), app_.get_home_uid(), td_dest, input.tag, reinterpret_cast<uintptr_t>(&input),
                          sizeof(input));
  // bookkeeper_.on_order_input(app_.now(), app_.get_home_uid(), find_td_location(source, account)->uid, input);
  return input.order_id;
}

uint64_t BacktestContext::insert_order_input(const std::string &source, const std::string &account,
                                             OrderInput &order_input) {
  auto insert_time = now();
  order_input.instrument_type = get_instrument_type(order_input.exchange_id, order_input.instrument_id);
  if (order_input.instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(order_input.instrument_type),
                 order_input.instrument_id, order_input.exchange_id);
    return 0;
  }
  uint32_t td_dest = find_td_location(source, account)->uid;
  auto &&writer = app_.get_writer(location::PUBLIC);
  order_input.order_id = order_input.order_id == 0 ? get_order_id(writer, td_dest) : order_input.order_id;
  order_input.insert_time = insert_time;
  writer->write_raw_at_as(now(), now(), app_.get_home_uid(), td_dest, order_input.tag,
                          reinterpret_cast<uintptr_t>(&order_input), sizeof(order_input));
  // bookkeeper_.on_order_input(app_.now(), app_.get_home_uid(), account_location_uid, input);
  return order_input.order_id;
}

uint64_t BacktestContext::insert_order_trigger(const std::string &instrument_id, const std::string &exchange_id,
                                               const std::string &source, const std::string &account,
                                               double limit_price, int64_t volume, PriceType type, Side side,
                                               Offset offset, OrderTriggerType trigger_type, double stop_price,
                                               HedgeFlag hedge_flag, bool is_swap) {
  return {};
}

std::vector<uint64_t> BacktestContext::insert_batch_orders(
    const std::string &source, const std::string &account, const std::vector<std::string> &instrument_ids,
    const std::vector<std::string> &exchange_ids, std::vector<double> limit_prices, std::vector<int64_t> volumes,
    std::vector<PriceType> types, std::vector<Side> sides, std::vector<Offset> offsets,
    std::vector<HedgeFlag> hedge_flags, std::vector<bool> is_swaps, const std::vector<std::string> &contract_ids) {
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
                     types.at(i), sides.at(i), offsets.at(i), contract_ids.at(i), hedge_flags.at(i), is_swaps.at(i));
    order_ids.push_back(order_id);
  }
  return order_ids;
}

std::vector<uint64_t> BacktestContext::insert_array_orders(const std::string &source, const std::string &account,
                                                           std::vector<OrderInput> &order_inputs) {
  std::vector<uint64_t> order_ids{};
  for (const OrderInput &input : order_inputs) {
    uint64_t order_id =
        insert_order(input.instrument_id, input.exchange_id, source, account, input.limit_price, input.volume,
                     input.price_type, input.side, input.offset, input.contract_id, input.hedge_flag, input.is_swap);
    order_ids.push_back(order_id);
  }
  return order_ids;
}

uint64_t BacktestContext::insert_algo_order(const std::string &instrument_id, const std::string &exchange_id,
                                            const std::string &source, const std::string &account, int64_t begin_time,
                                            int64_t end_time, int64_t volume, PriceType type, Side side, Offset offset,
                                            const std::string &algo_type_id, const std::string &algo_id,
                                            const std::string &args, bool is_local, uint32_t basket_uid,
                                            longfist::enums::PriceLevel price_level, double price_offset) {
  return {};
}

uint64_t BacktestContext::update_algo_order_volume(uint64_t origin_order_id, const std::string &source,
                                                   const std::string &account, int64_t volume) {
  return {};
}

uint64_t BacktestContext::cancel_order(uint64_t order_id, OrderActionFlag action_flag) {
  auto account_uid = app_.get_home_uid();
  uint32_t account_location_uid = (order_id >> 32u) xor (app_.get_home_uid());
  if (not app_.has_location(account_location_uid)) {
    SPDLOG_ERROR("no writer for [{:08x}]", account_location_uid);
  }
  auto &&writer = app_.get_writer(location::PUBLIC);
  OrderAction action{};

  action.order_action_id = writer->current_frame_uid();
  action.order_id = order_id;
  action.action_flag = action_flag;

  writer->write_raw_at_as(now(), now(), app_.get_home_uid(), account_location_uid, action.tag,
                          reinterpret_cast<uintptr_t>(&action), sizeof(action));
  return action.order_action_id;
}

uint64_t BacktestContext::cancel_order_trigger(uint64_t trigger_id) { return {}; }

uint64_t BacktestContext::cancel_algo_order(uint64_t algo_order_id, longfist::enums::AlgoOrderActionFlag action_flag) {
  return {};
}

uint64_t BacktestContext::toggle_algo_order(uint64_t algo_order_id, longfist::enums::AlgoOrderActionFlag action_flag) {
  return {};
}

broker::Client &BacktestContext::get_broker_client() { return broker_client_; }

book::Bookkeeper &BacktestContext::get_bookkeeper() { return bookkeeper_; }

void BacktestContext::req_history_order(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_history_trade(const std::string &source, const std::string &account, uint32_t query_num) {}

void BacktestContext::req_deregister() { app_.request_deregister(); }

void BacktestContext::update_strategy_state(StrategyStateUpdate &state_update) {}

location_ptr BacktestContext::get_location(uint32_t location_uid) { return app_.get_location(location_uid); }

location_ptr BacktestContext::find_td_location(const std::string &source, const std::string &account,
                                               bool check_exist) const {
  auto td_location =
      location::make_shared(longfist::enums::mode::BACKTEST, category::TD, source, account, app_.get_locator());
  if (check_exist) {
    if (not app_.has_location(td_location->uid)) {
      SPDLOG_ERROR(fmt::format("invalid account {}_{}", source, account));
      return app_.get_location(td_location->uid);
    }
  }
  return td_location;
}

uint64_t BacktestContext::get_order_id(const writer_ptr &writer, uint32_t dest) const {
  uint32_t id_part = static_cast<uint32_t>(writer->current_frame_uid());
  uint64_t dest_part = static_cast<uint64_t>(get_home_uid() xor dest) << 32u;
  return dest_part | id_part;
}

uint32_t BacktestContext::get_home_uid() const { return app_.get_home_uid(); }

const std::string BacktestContext::get_config() const { return "{}"; }
} // namespace kungfu::wingchun::strategy