#include <fmt/format.h>
#include <kungfu/wingchun/strategy/replay.h>
#include <kungfu/yijinjing/journal/page.h>
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

ReplayContext::ReplayContext(practice::apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app_), bookkeeper_(app_, broker_client_),
      reader_for_write_(std::make_shared<reader>(true, false, std::make_shared<bus>(false))) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
  auto live_home = app_.get_live_home();
  for (auto dest_id : live_home->locator->list_location_dest(live_home)) {
    if (page::check_page_existed(live_home, dest_id)) {
      reader_for_write_->join(live_home, dest_id, app_.get_begin_time());
    } else {
      SPDLOG_WARN("page not existed, source_location: {}, dest: {}", live_home->uname, (uint32_t)dest_id);
    }
  }
}

void ReplayContext::on_start() {
  SPDLOG_DEBUG("arguments_: {}", get_arguments());
  if (not get_arguments().empty()) {
    auto config = nlohmann::json::parse(get_arguments());
    if (config.value<bool>("bypass_accounting", false)) {
      bypass_accounting();
    }
  }

  broker_client_.on_start(events_);
  if (not is_bypass_accounting()) {
    bookkeeper_.on_start(events_);
  }

  events_ | $$(on_timer_check());
}

void ReplayContext::prepare(const event_ptr &event) {
  if (event->msg_type() == Position::tag) {
    const Position &position = event->data<Position>();
    if (position.holder_uid == get_live_home_uid()) {
      get_broker_client().subscribe(position.exchange_id, position.instrument_id);
    }
  }

  if (not broker_client_.enrolled_td_ready() or not broker_client_.enrolled_md_ready() or
      not broker_client_.enrolled_operator_ready()) {
    return;
  }

  if (not broker_client_.has_enrolled_td_channel(get_live_home_uid())) {
    return;
  }

  auto ledger_uid = app_.get_ledger_home_location()->uid;
  if (event->msg_type() == PositionEnd::tag and event->source() == ledger_uid) {
    positions_set_ = true;
  }
  if (not positions_set_) {
    return;
  }

  get_bookkeeper().guard_positions();
  started_ = true;
}

bool ReplayContext::is_started() const { return started_; }

uint32_t ReplayContext::get_home_uid() const { return app_.get_home_uid(); }

uint32_t ReplayContext::get_live_home_uid() const { return app_.get_live_home_uid(); }

int64_t ReplayContext::now() const { return app_.now(); }

int32_t ReplayContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  return app_.add_timer(nanotime, callback);
}

int32_t ReplayContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  return app_.add_time_interval(duration, callback);
}

void ReplayContext::clear_timer(int32_t timer_id) { app_.clear_timer(timer_id); }

void ReplayContext::add_account(const std::string &source, const std::string &account) {
  auto home = app_.get_live_home();
  auto account_location = location::make_shared(mode::LIVE, category::TD, source, account, home->locator);
  if (not app_.has_location(account_location->uid)) {
    SPDLOG_ERROR(fmt::format("invalid account {}_{}", source, account));
    throw wingchun_error(fmt::format("invalid account {}_{}", source, account));
  }

  broker_client_.enroll_td(account_location);
}

void ReplayContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                              const std::string &exchange_ids) {
  auto md_location = broker_client_.find_md_location(source, app_.get_live_home());
  for (const auto &instrument_id : instrument_ids) {
    broker_client_.subscribe(md_location, exchange_ids, instrument_id);
  }
}

void ReplayContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                  uint64_t data_type) {
  auto md_location = broker_client_.find_md_location(source, app_.get_live_home());
  broker_client_.subscribe_all(md_location, market_type, instrument_type, data_type);
}

void ReplayContext::subscribe_operator(const std::string &group, const std::string &name) {
  auto home = app_.get_live_home();
  auto operator_location = location::make_shared(mode::LIVE, category::OPERATOR, group, name, home->locator);
  if (not app_.has_location(operator_location->uid)) {
    throw wingchun_error(fmt::format("invalid operator {}_{}", group, name));
  }

  broker_client_.enroll_operator(operator_location);
}

broker::Client &ReplayContext::get_broker_client() { return broker_client_; }

book::Bookkeeper &ReplayContext::get_bookkeeper() { return bookkeeper_; }

uint64_t ReplayContext::insert_block_message(const std::string &source, const std::string &account,
                                             const std::string &opponent_seat, uint64_t match_number,
                                             bool is_specific) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }
  auto frame = read_next(BlockMessage::tag);
  return frame->data<BlockMessage>().block_id;
}

uint64_t ReplayContext::insert_order_trigger(const std::string &instrument_id, const std::string &exchange_id,
                                             const std::string &source, const std::string &account, double limit_price,
                                             int64_t volume, PriceType type, Side side, Offset offset,
                                             OrderTriggerType trigger_type, double stop_price, HedgeFlag hedge_flag,
                                             bool is_swap) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }

  auto frame = read_next(OrderTriggerInput::tag);
  return frame->data<OrderTriggerInput>().trigger_id;
}

uint64_t ReplayContext::insert_order(const std::string &instrument_id, const std::string &exchange_id,
                                     const std::string &source, const std::string &account, double limit_price,
                                     int64_t volume, PriceType type, Side side, Offset offset, HedgeFlag hedge_flag,
                                     bool is_swap, uint64_t block_id, uint64_t parent_id) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(OrderInput::tag);
  auto &input = frame->data<OrderInput>();
  if (not is_bypass_accounting()) {
    bookkeeper_.on_order_input(now(), get_live_home_uid(), account_location_uid, input);
  }
  return input.order_id;
}

uint64_t ReplayContext::insert_order_input(const std::string &source, const std::string &account,
                                           OrderInput &order_input) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(OrderInput::tag);
  auto &input = frame->data<OrderInput>();

  if (not is_bypass_accounting()) {
    bookkeeper_.on_order_input(now(), get_live_home_uid(), account_location_uid, input);
  }

  return input.order_id;
}

std::vector<uint64_t> ReplayContext::insert_batch_orders(
    const std::string &source, const std::string &account, const std::vector<std::string> &instrument_ids,
    const std::vector<std::string> &exchange_ids, std::vector<double> limit_prices, std::vector<int64_t> volumes,
    std::vector<PriceType> types, std::vector<Side> sides, std::vector<Offset> offsets,
    std::vector<HedgeFlag> hedge_flags, std::vector<bool> is_swaps) {
  std::vector<uint64_t> order_ids{};
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return order_ids;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return order_ids;
  }

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

std::vector<uint64_t> ReplayContext::insert_array_orders(const std::string &source, const std::string &account,
                                                         std::vector<OrderInput> &order_inputs) {
  std::vector<uint64_t> order_ids{};
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return order_ids;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return order_ids;
  }

  for (const OrderInput &input : order_inputs) {
    uint64_t order_id =
        insert_order(input.instrument_id, input.exchange_id, source, account, input.limit_price, input.volume,
                     input.price_type, input.side, input.offset, input.hedge_flag, input.is_swap);
    order_ids.push_back(order_id);
  }

  return order_ids;
}

uint64_t ReplayContext::insert_algo_order(const std::string &instrument_id, const std::string &exchange_id, const 
                                          const std::string &source, const std::string &account, int64_t begin_time,
                                          int64_t end_time, int64_t volume, PriceType type, Side side, Offset offset,
                                          const std::string &algo_type_id, const std::string &algo_id,
                                          const std::string &args, bool is_local,  uint32_t basket_uid) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(AlgoOrderInput::tag);
  return frame->data<AlgoOrderInput>().order_id;
}


uint64_t ReplayContext::update_algo_order(uint64_t origin_order_id,const std::string &instrument_id, const std::string &exchange_id, const 
                                          const std::string &source, const std::string &account, int64_t begin_time,
                                          int64_t end_time, int64_t volume, PriceType type, Side side, Offset offset,
                                          const std::string &algo_type_id, const std::string &algo_id,
                                          const std::string &args, bool is_local,  uint32_t basket_uid) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  auto account_location_uid = broker_client_.get_td_location_uid(source, account);
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(AlgoOrderInput::tag);
  return frame->data<AlgoOrderInput>().order_id;
}

uint64_t ReplayContext::cancel_order(uint64_t order_id, OrderActionFlag action_flag) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  uint32_t account_location_uid = (order_id >> 32u) xor (get_live_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(OrderAction::tag);
  return frame->data<OrderAction>().order_action_id;
}

uint64_t ReplayContext::cancel_order_trigger(uint64_t trigger_id) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  uint32_t account_location_uid = (trigger_id >> 32u) xor (get_live_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(OrderTriggerAction::tag);
  return frame->data<OrderTriggerAction>().order_trigger_action_id;
}

uint64_t ReplayContext::cancel_algo_order(uint64_t algo_order_id, AlgoOrderActionFlag action_flag) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  uint32_t account_location_uid = (algo_order_id >> 32u) xor (get_live_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(AlgoOrderAction::tag);
  return frame->data<AlgoOrderAction>().order_action_id;
}

uint64_t ReplayContext::toggle_algo_order(uint64_t algo_order_id, longfist::enums::AlgoOrderActionFlag action_flag) {
  if (not is_started()) {
    SPDLOG_ERROR("context not ready");
    return 0;
  }

  uint32_t account_location_uid = (algo_order_id >> 32u) xor (get_live_home_uid());
  if (not broker_client_.is_ready(account_location_uid)) {
    SPDLOG_ERROR("toggle_algo_order account {} not ready", app_.get_location_uname(account_location_uid));
    return 0;
  }

  auto frame = read_next(AlgoOrderAction::tag);
  return frame->data<AlgoOrderAction>().order_action_id;
}

void ReplayContext::req_history_order(const std::string &source, const std::string &account, uint32_t query_num) {}

void ReplayContext::req_history_trade(const std::string &source, const std::string &account, uint32_t query_num) {}

void ReplayContext::req_deregister() {
  SPDLOG_WARN("req_deregister");
  app_.request_deregister();
}

void ReplayContext::update_strategy_state(StrategyStateUpdate &state_update) {}

yijinjing::data::location_ptr ReplayContext::get_location(uint32_t location_uid) {
  return app_.get_location(location_uid);
}

frame_ptr ReplayContext::read_next(uint32_t msg_type) {
  while (reader_for_write_->data_available()) {
    auto frame = reader_for_write_->current_frame();
    if (frame->msg_type() == msg_type && frame->gen_time() >= now()) {
      break;
    }
    reader_for_write_->next();
  }

  if (not reader_for_write_->data_available()) {
    req_deregister();
    SPDLOG_ERROR("no more data available");
    throw wingchun_error("no more data available");
  }

  return reader_for_write_->current_frame();
}

void ReplayContext::on_timer_check() {
  if (now() >= app_.get_end_time()) {
    SPDLOG_WARN("end time limit exceeded");
    req_deregister();
  }
}

void ReplayContext::set_resume_policy(longfist::enums::ResumePolicy resume_policy) {
  broker_client_.set_resume_policy(resume_policy);
}

longfist::enums::ResumePolicy ReplayContext::get_resume_policy() { return broker_client_.get_resume_policy_value(); }

} // namespace kungfu::wingchun::strategy
