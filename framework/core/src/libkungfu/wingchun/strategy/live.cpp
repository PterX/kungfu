// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020-07-20.
//

#include <fmt/format.h>

#include <kungfu/wingchun/strategy/live.h>
#include <kungfu/yijinjing/log.h>
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

namespace kungfu::wingchun::strategy {

LiveContext::LiveContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app_), bookkeeper_(app_, broker_client_), basketorder_engine_(app_) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void LiveContext::on_start() {
  SPDLOG_DEBUG("arguments_: {}", arguments_);
  if (not arguments_.empty()) {
    auto config = nlohmann::json::parse(arguments_);
    if (config.value<bool>("bypass_accounting", false)) {
      bypass_accounting();
    }
  }

  broker_client_.on_start(events_);
  if (not is_bypass_accounting()) {
    bookkeeper_.on_start(events_);
  }
  basketorder_engine_.on_start(events_);
}

bool LiveContext::is_started() const { return started_; }

void LiveContext::prepare(const event_ptr &event) {
  if (event->msg_type() == Position::tag) {
    const Position &position = event->data<Position>();
    if (position.holder_uid == app_.get_home_uid()) {
      get_broker_client().subscribe(position.exchange_id, position.instrument_id);
    }
  }

  auto ledger_uid = app_.get_ledger_home_location()->uid;
  if (not app_.has_writer(ledger_uid)) {
    return;
  }
  auto writer = app_.get_writer(ledger_uid);

  auto connected_test = [&](const auto &locations) {
    return std::all_of(locations.begin(), locations.end(),
                       [&](const auto &it) { return get_broker_client().is_connected(it.second->uid); });
  };
  if (not broker_states_requested_ and connected_test(list_accounts()) and connected_test(list_md()) and
      connected_test(list_op())) {
    writer->mark(now(), BrokerStateRequest::tag);
    writer->mark(now(), OperatorStateRequest::tag);
    broker_states_requested_ = true;
  }

  auto ready_test = [&](const auto &locations) {
    return std::all_of(locations.begin(), locations.end(),
                       [&](const auto &it) { return get_broker_client().is_ready(it.second->uid); });
  };
  if (not ready_test(list_accounts()) or not ready_test(list_md()) or not ready_test(list_op())) {
    return;
  }

  auto has_td_channel = [&](const auto &locations) {
    return std::all_of(locations.begin(), locations.end(), [&](const auto &it) {
      return get_broker_client().has_channel(get_home_uid(), it.second->uid) and
             get_broker_client().has_channel(it.second->uid, get_home_uid());
    });
  };
  if (not has_td_channel(list_accounts())) {
    return;
  }

  if (not positions_requested_) {
    if (not is_book_held()) {
      // Start - Let ledger prepare book for strategy
      writer->mark(now(), KeepPositionsRequest::tag);
      writer->mark(now(), ResetBookRequest::tag);
    }

    for (const auto &td_pair : list_accounts()) {
      writer->write(now(), td_pair.second->to<OutputKey>());
    }

    for (const auto &pair : get_broker_client().get_instrument_keys()) {
      writer->write(now(), pair.second);
    }
    if (is_positions_mirrored()) {
      writer->mark(now(), MirrorPositionsRequest::tag);
    }
    // End - Let ledger prepare book for strategy
    if (not is_book_held() and not is_positions_mirrored()) {
      writer->mark(now(), RebuildPositionsRequest::tag);
    }
    // Request ledger to recover book for strategy
    writer->mark(now(), AssetRequest::tag);
    writer->mark(now(), PositionRequest::tag);
    positions_requested_ = true;
    return;
  }
  if (event->msg_type() == PositionEnd::tag and event->source() == ledger_uid) {
    positions_set_ = true;
  }
  if (not positions_set_) {
    return;
  }
  get_bookkeeper().guard_positions();
  started_ = true;
}

int64_t LiveContext::now() const { return app_.now(); }

void LiveContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  app_.add_timer(nanotime, callback);
}

void LiveContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  app_.add_time_interval(duration, callback);
}

void LiveContext::add_account(const std::string &source, const std::string &account) {
  uint32_t hashed_account = hash_account(source, account);

  if (td_locations_.find(hashed_account) != td_locations_.end()) {
    SPDLOG_ERROR(fmt::format("duplicated account {}_{}", source, account));
  }

  auto home = app_.get_io_device()->get_home();
  auto account_location = location::make_shared(mode::LIVE, category::TD, source, account, home->locator);
  if (home->mode == mode::LIVE and not app_.has_location(account_location->uid)) {
    SPDLOG_ERROR(fmt::format("invalid account {}_{}", source, account));
  }

  td_locations_.emplace(hashed_account, account_location);
  td_locations_.emplace(account_location->uid, account_location);
  account_location_ids_.emplace(hashed_account, account_location->uid);

  broker_client_.enroll_account(account_location);
}

void LiveContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                            const std::string &exchange_ids) {
  auto md_location = find_md_location(source);
  for (const auto &instrument_id : instrument_ids) {
    broker_client_.subscribe(md_location, exchange_ids, instrument_id);
  }
  md_locations_.emplace(md_location->uid, md_location);
  ensure_connect();
  send_instrument_keys();
}

void LiveContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                uint64_t data_type) {
  broker_client_.subscribe_all(find_md_location(source), market_type, instrument_type, data_type);
  ensure_connect();
  send_instrument_keys();
}

void LiveContext::subscribe_operator(const std::string &group, const std::string &name) {
  uint32_t hashed_op = hash_operator(group, name);

  if (op_locations_.find(hashed_op) != op_locations_.end()) {
    throw wingchun_error(fmt::format("duplicated operator subscribed {}_{}", group, name));
  }

  auto home = app_.get_home();
  auto operator_location = location::make_shared(mode::LIVE, category::OPERATOR, group, name, home->locator);
  if (home->mode == mode::LIVE and not app_.has_location(operator_location->uid)) {
    throw wingchun_error(fmt::format("invalid operator {}_{}", group, name));
  }

  // op_locations_.emplace(hashed_op, operator_location);
  op_locations_.emplace(operator_location->uid, operator_location);

  broker_client_.enroll_operator(operator_location);
}

uint64_t LiveContext::insert_block_message(const std::string &source, const std::string &account,
                                           const std::string &opponent_seat, uint64_t match_number, bool is_specific) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  BlockMessage &msg = writer->open_data<BlockMessage>(app_.now());
  strncpy(msg.opponent_seat, opponent_seat.c_str(), opponent_seat.length());
  msg.match_number = match_number;
  msg.is_specific = is_specific;
  msg.block_id = writer->current_frame_uid();
  uint64_t block_id = msg.block_id;
  writer->close_data();
  return block_id;
}

uint64_t LiveContext::insert_order_trigger(const std::string &instrument_id, const std::string &exchange_id,
                                           const std::string &source, const std::string &account, double limit_price,
                                           int64_t volume, longfist::enums::PriceType type, longfist::enums::Side side,
                                           longfist::enums::Offset offset,
                                           longfist::enums::OrderTriggerType trigger_type,
                                           longfist::enums::TimeCondition time_condition,
                                           longfist::enums::ParkedType parked_type, double stop_price,
                                           longfist::enums::HedgeFlag hedge_flag, bool is_swap) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderTriggerInput &input = writer->open_data<OrderTriggerInput>(app_.now());
  input.trigger_id = writer->current_frame_uid();
  strcpy(input.instrument_id, instrument_id.c_str());
  strcpy(input.exchange_id, exchange_id.c_str());
  input.instrument_type = instrument_type;
  input.limit_price = limit_price;
  input.frozen_price = limit_price;
  input.volume = volume;
  input.stop_price = stop_price;
  input.price_type = type;
  input.side = side;
  input.offset = offset;
  input.hedge_flag = hedge_flag;
  input.is_swap = is_swap;
  input.time_condition = time_condition;
  input.parked_type = parked_type;
  input.insert_time = time::now_in_nano();
  writer->close_data();
  return input.trigger_id;
}

uint64_t LiveContext::insert_order(const std::string &instrument_id, const std::string &exchange_id,
                                   const std::string &source, const std::string &account, double limit_price,
                                   int64_t volume, PriceType type, Side side, Offset offset, HedgeFlag hedge_flag,
                                   bool is_swap, uint64_t block_id, uint64_t parent_id) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderInput &input = writer->open_data<OrderInput>(app_.now());
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
  input.parent_id = parent_id;
  input.is_swap = is_swap;
  input.insert_time = time::now_in_nano();
  writer->close_data();
  if (not is_bypass_accounting()) {
    bookkeeper_.on_order_input(app_.now(), app_.get_home_uid(), account_location_uid, input);
  }
  return input.order_id;
}

uint64_t LiveContext::insert_order_input(const std::string &source, const std::string &account,
                                         longfist::types::OrderInput &order_input) {

  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  order_input.instrument_type = get_instrument_type(order_input.exchange_id, order_input.instrument_id);
  if (order_input.instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(order_input.instrument_type),
                 order_input.instrument_id, order_input.exchange_id);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderInput &input = writer->open_data<OrderInput>(app_.now());
  order_input.order_id = order_input.order_id == 0 ? writer->current_frame_uid() : order_input.order_id;
  order_input.insert_time = time::now_in_nano();
  memcpy(&input, &order_input, sizeof(input));
  writer->close_data();
  if (not is_bypass_accounting()) {
    bookkeeper_.on_order_input(app_.now(), app_.get_home_uid(), account_location_uid, order_input);
  }
  return order_input.order_id;
}

std::vector<uint64_t>
LiveContext::insert_batch_orders(const std::string &source, const std::string &account,
                                 const std::vector<std::string> &instrument_ids,
                                 const std::vector<std::string> &exchange_ids, std::vector<double> limit_prices,
                                 std::vector<int64_t> volumes, std::vector<longfist::enums::PriceType> types,
                                 std::vector<longfist::enums::Side> sides, std::vector<longfist::enums::Offset> offsets,
                                 std::vector<longfist::enums::HedgeFlag> hedge_flags, std::vector<bool> is_swaps) {
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

  auto account_location_uid = get_td_location_uid(source, account);
  auto writer = app_.get_writer(account_location_uid);
  writer->mark(time::now_in_nano(), BatchOrderBegin::tag);

  for (int i = 0; i < instrument_ids.size(); ++i) {
    uint64_t order_id =
        insert_order(instrument_ids.at(i), exchange_ids.at(i), source, account, limit_prices.at(i), volumes.at(i),
                     types.at(i), sides.at(i), offsets.at(i), hedge_flags.at(i), is_swaps.at(i));
    order_ids.push_back(order_id);
  }

  writer->mark(time::now_in_nano(), BatchOrderEnd::tag);
  writer->close_data();
  return order_ids;
}

std::vector<uint64_t> LiveContext::insert_array_orders(const std::string &source, const std::string &account,
                                                       std::vector<longfist::types::OrderInput> &order_inputs) {
  std::vector<uint64_t> order_ids{};
  auto account_location_uid = get_td_location_uid(source, account);
  auto writer = app_.get_writer(account_location_uid);
  writer->mark(time::now_in_nano(), BatchOrderBegin::tag);

  for (const OrderInput &input : order_inputs) {
    uint64_t order_id =
        insert_order(input.instrument_id, input.exchange_id, source, account, input.limit_price, input.volume,
                     input.price_type, input.side, input.offset, input.hedge_flag, input.is_swap);
    order_ids.push_back(order_id);
  }

  writer->mark(time::now_in_nano(), BatchOrderEnd::tag);
  writer->close_data();
  return order_ids;
}

uint64_t LiveContext::insert_basket_order(uint64_t basket_id, const std::string &source, const std::string &account,
                                          longfist::enums::Side side, longfist::enums::PriceType price_type,
                                          longfist::enums::PriceLevel price_level, double price_offset,
                                          int64_t volume) {
  auto account_location_uid = get_td_location_uid(source, account);
  auto insert_time = time::now_in_nano();
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }

  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  BasketOrder &input = writer->open_data<BasketOrder>(app_.now());
  input.order_id = writer->current_frame_uid();
  input.parent_id = basket_id;
  input.source_id = app_.get_home_uid();
  input.dest_id = account_location_uid;
  input.side = side;
  input.price_type = price_type;
  input.price_level = price_level;
  input.price_offset = price_offset;
  input.volume = volume;
  input.insert_time = insert_time;
  input.calculation_mode =
      input.volume == VOLUME_ZERO ? BasketOrderCalculationMode::Dynamic : BasketOrderCalculationMode::Static;
  writer->close_data();
  basketorder_engine_.insert_basket_order(app_.now(), input);
  return input.order_id;
}

uint64_t LiveContext::insert_algo_order(const std::string &instrument_id, const std::string &exchange_id,
                                        const std::string &source, const std::string &account, int64_t begin_time,
                                        int64_t end_time, int64_t volume, longfist::enums::PriceType type,
                                        longfist::enums::Side side, longfist::enums::Offset offset,
                                        const std::string &algo_type_id, const std::string &algo_id,
                                        const std::string &args, bool is_local) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto now = app_.now();
  auto writer = app_.get_writer(account_location_uid);
  AlgoOrderInput input = {};
  input.order_id = writer->current_frame_uid();
  input.insert_time = now;
  input.begin_time = begin_time;
  input.end_time = end_time;
  strcpy(input.instrument_id, instrument_id.c_str());
  strcpy(input.exchange_id, exchange_id.c_str());
  input.instrument_type = get_instrument_type(exchange_id, instrument_id);
  input.side = side;
  input.offset = offset;
  input.price_type = type;
  input.volume = volume;
  strcpy(input.algo_type_id, algo_type_id.c_str());
  strcpy(input.algo_id, algo_id.c_str());
  input.args = args;
  input.is_local = is_local;

  writer->write(now, input);
  return input.order_id;
}

uint64_t LiveContext::cancel_order(uint64_t order_id, OrderActionFlag action_flag) {
  uint32_t account_location_uid = (order_id >> 32u) xor (app_.get_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderAction &action = writer->open_data<OrderAction>(0);

  action.order_action_id = writer->current_frame_uid();
  action.order_id = order_id;
  action.action_flag = action_flag;

  writer->close_data();
  return action.order_action_id;
}

uint64_t LiveContext::cancel_order_trigger(uint64_t trigger_id) {
  uint32_t account_location_uid = (trigger_id >> 32u) xor (app_.get_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderTriggerAction &action = writer->open_data<OrderTriggerAction>(0);

  action.order_trigger_action_id = writer->current_frame_uid();
  action.trigger_id = trigger_id;
  action.action_flag = OrderActionFlag::Cancel;

  writer->close_data();
  return action.order_trigger_action_id;
}

uint64_t LiveContext::cancel_algo_order(uint64_t algo_order_id) {
  uint32_t account_location_uid = (algo_order_id >> 32u) xor (app_.get_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", td_locations_.at(account_location_uid)->uname);
    return 0;
  }

  auto account_location = app_.get_location(account_location_uid);
  auto writer = app_.get_writer(account_location_uid);
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  AlgoOrderAction &action = writer->open_data<AlgoOrderAction>(0);
  action.order_action_id = writer->current_frame_uid();
  action.order_id = algo_order_id;
  action.action_flag = OrderActionFlag::Cancel;
  writer->close_data();
  return action.order_action_id;
}

const location_map &LiveContext::list_md() const { return md_locations_; }

const location_map &LiveContext::list_op() const { return op_locations_; }

const location_map &LiveContext::list_accounts() const { return td_locations_; }

broker::Client &LiveContext::get_broker_client() { return broker_client_; }

book::Bookkeeper &LiveContext::get_bookkeeper() { return bookkeeper_; }

[[maybe_unused]] uint32_t LiveContext::lookup_account_location_id(const std::string &account) const {
  return account_location_ids_.at(hash_str_32(account));
}
basketorder::BasketOrderEngine &LiveContext::get_basketorder_engine() { return basketorder_engine_; }

uint32_t LiveContext::get_td_location_uid(const std::string &source, const std::string &account) const {
  uint32_t hashed_account = hash_account(source, account);
  if (td_locations_.find(hashed_account) == td_locations_.end()) {
    SPDLOG_ERROR(fmt::format("invalid account {}_{}", source, account));
  }

  return td_locations_.at(hashed_account)->uid;
}

const location_ptr &LiveContext::find_md_location(const std::string &source) {
  if (market_data_.find(source) == market_data_.end()) {
    auto home = app_.get_home();
    auto md_location = location::make_shared(mode::LIVE, category::MD, source, source, home->locator);
    if (not app_.has_location(md_location->uid)) {
      SPDLOG_ERROR(fmt::format("invalid md {}", source));
    }
    market_data_.emplace(source, md_location);
  }
  return market_data_.at(source);
}

void LiveContext::req_history_order(const std::string &source, const std::string &account, uint32_t query_num) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {}_{} not ready", source, account);
    return;
  }
  auto writer = app_.get_writer(account_location_uid);
  RequestHistoryOrder &request = writer->open_data<RequestHistoryOrder>();
  request.trigger_time = now();
  request.query_num = query_num;
  writer->close_data();
}

void LiveContext::req_history_trade(const std::string &source, const std::string &account, uint32_t query_num) {
  auto account_location_uid = get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {}_{} not ready", source, account);
    return;
  }
  auto writer = app_.get_writer(account_location_uid);
  RequestHistoryTrade &request = writer->open_data<RequestHistoryTrade>();
  request.trigger_time = now();
  request.query_num = query_num;
  writer->close_data();
}

void LiveContext::req_deregister() { app_.request_deregister(); }

void LiveContext::update_strategy_state(StrategyStateUpdate &state_update) {
  auto writer = app_.get_writer(location::PUBLIC);
  state_update.update_time = now();
  writer->write(state_update.update_time, state_update);
}

void LiveContext::ensure_connect() {
  if (not is_started()) {
    return;
  }

  const event_ptr &e = app_.get_reader()->current_frame();
  for (const auto &pair : app_.get_registry()) {
    SPDLOG_DEBUG("Register: {}", pair.second.to_string());
    broker_client_.connect(e, pair.second);
  }

  for (const auto &pair : app_.get_bands()) {
    SPDLOG_DEBUG("Band: {}", pair.second.to_string());
    broker_client_.connect(e, pair.second);
  }
}

void LiveContext::send_instrument_keys() {
  if (not is_started()) {
    return;
  }
  for (const auto &pair : app_.get_locations()) {
    SPDLOG_DEBUG("Location: {}", pair.second->to_string());
    broker_client_.try_renew(app_.now(), pair.second);
  }
}

yijinjing::data::location_ptr LiveContext::get_location(uint32_t location_uid) {
  return app_.get_location(location_uid);
}

uint32_t LiveContext::get_home_uid() const { return app_.get_home_uid(); }

} // namespace kungfu::wingchun::strategy
