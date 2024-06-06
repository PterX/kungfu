// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/1/14.
//

#include "watcher.h"
#include "commission_store.h"
#include "config_store.h"
#include "history.h"
#include <kungfu/yijinjing/cache/cached.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/util/os.h>
#include <sstream>

using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::map;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::cache;
using namespace kungfu::yijinjing::data;

namespace kungfu::node {

constexpr uint32_t STEP_INTERVAL = 10;
constexpr uint32_t REFRESH_REQUIRED_DATA_LIMIT_BY_SYNC = 3000;

inline std::string format(uint32_t uid) { return fmt::format("{:08x}", uid); }

Napi::FunctionReference Watcher::constructor = {};

inline location_ptr GetWatcherLocation(const Napi::CallbackInfo &info) {
  if (not IsValid(info, 0, &Napi::Value::IsString)) {
    throw Napi::Error::New(info.Env(), "Invalid runtime dirname");
  }

  if (not IsValid(info, 1, &Napi::Value::IsString)) {
    throw Napi::Error::New(info.Env(), "Invalid node app name");
  }

  auto runtime_dir = info[0].As<Napi::String>().Utf8Value();
  auto name = info[1].As<Napi::String>().Utf8Value();
  auto result =
      std::make_shared<location>(mode::LIVE, category::SYSTEM, "node", name, IODevice::GetRuntimeLocator(runtime_dir));
  log::copy_log_settings(result, result->name);
  return result;
}

inline bool GetBypassRestore(const Napi::CallbackInfo &info) {
  if (not IsValid(info, 2, &Napi::Value::IsBoolean)) {
    throw Napi::Error::New(info.Env(), "Invalid bypassRestore argument");
  }
  return info[2].As<Napi::Boolean>().Value();
}

WatcherAutoClient::WatcherAutoClient(yijinjing::practice::apprentice &app, bool bypass_trading_data)
    : SilentAutoClient(app), bypass_trading_data_(bypass_trading_data) {}

void WatcherAutoClient::connect(const event_ptr &event, const longfist::types::Register &register_data) {
  auto app_uid = register_data.location_uid;
  if (not app_.has_location(app_uid)) {
    SPDLOG_WARN("no location {}", app_uid);
    return;
  }
  auto app_location = app_.get_location(app_uid);
  auto resume_time_point = get_resume_policy()->get_connect_time(app_, register_data);

  if (app_location->category == category::SYSTEM and should_connect_system(app_location)) {
    app_.request_read_from_public(app_.now(), app_uid, 0);
    app_.request_read_from(app_.now(), app_uid, app_.now());
    app_.request_write_to(app_.now(), app_uid);
    SPDLOG_INFO("resume {} connection from {}", app_.get_location_uname(app_uid), time::strftime(resume_time_point));
    return;
  }

  if (bypass_trading_data_) {
    if (app_location->category == category::MD and should_connect_md(app_location)) {
      app_.request_write_to(app_.now(), app_uid);
      SPDLOG_INFO("resume {} connection from {}", app_.get_location_uname(app_uid), time::strftime(resume_time_point));
    }

    if (app_location->category == category::TD and should_connect_td(app_location)) {
      app_.request_write_to(app_.now(), app_uid);
      SPDLOG_INFO("resume {} connection from {}", app_.get_location_uname(app_uid), time::strftime(resume_time_point));
    }

    if (app_location->category == category::STRATEGY and should_connect_strategy(app_location)) {
      app_.request_write_to(app_.now(), app_location->uid);
      SPDLOG_INFO("resume {} connection from {}", app_.get_location_uname(app_uid), time::strftime(resume_time_point));
    }

    if (app_location->category == category::OPERATOR and should_connect_operator(app_location)) {
      app_.request_write_to(app_.now(), app_location->uid);
      SPDLOG_INFO("resume {} connection from {}", app_.get_location_uname(app_uid), time::strftime(resume_time_point));
    }

  } else {
    wingchun::broker::SilentAutoClient::connect(event, register_data);
  }
}

void WatcherAutoClient::connect(const event_ptr &event, const longfist::types::Band &band) { return; }

bool WatcherAutoClient::should_connect_system(const yijinjing::data::location_ptr &system_location) const {
  if (system_location->group == "service") {
    return true;
  }
  return false;
}

Watcher::Watcher(const Napi::CallbackInfo &info)
    : ObjectWrap(info),                                    //
      apprentice(GetWatcherLocation(info), true),          //
      bypass_accounting_(GetBool(info, 3)),                //
      bypass_trading_data_(GetBool(info, 4)),              //
      refresh_trading_data_before_sync_(GetBool(info, 5)), //
      bypass_refresh_book_(GetBool(info, 6)),              //
      milliseconds_sleep_after_step_(GetNumber(info, 7)),  //
      trading_data_reader_(std::make_shared<yijinjing::journal::reader>(
          true, false, std::make_shared<yijinjing::journal::bus>(false))),                        //
      broker_client_(*this, bypass_trading_data_),                                                //
      bookkeeper_(*this, broker_client_, bypass_accounting_, true),                               //
      ledger_ref_(Napi::ObjectReference::New(Napi::Object::New(info.Env()), 1)),                  //
      app_states_ref_(Napi::ObjectReference::New(Napi::Object::New(info.Env()), 1)),              //
      strategy_states_ref_(Napi::ObjectReference::New(Napi::Object::New(info.Env()), 1)),         //
      config_ref_(Napi::ObjectReference::New(ConfigStore::NewInstance({info[0]}).ToObject(), 1)), //
      update_ledger(ledger_ref_),                                                                 //
      reset_cache(*this, ledger_ref_) {                                                           //

  serialize::InitStateMap(info, ledger_ref_, "ledger");

  auto today = time::restore_start();
  auto config_store = ConfigStore::Unwrap(config_ref_.Value());

  bool sync_schema = not get_io_device()->is_usable();
  if (sync_schema) {
    config_store->profile_.setup();
  }

  SPDLOG_INFO("Watcher created for {}", get_home_uname());

  // byPassRestore will be true after ui browserWindow reopen by crashed
  if (GetBypassRestore(info) or bypass_trading_data_) {
    return;
  }

  for (const auto &item : config_store->profile_.get_all(Location{})) {
    auto saved_location = location::make_shared(item, get_locator());
    add_location(now(), saved_location);
    RestoreState(saved_location, today, INT64_MAX, sync_schema);
    // for hidden pos && asset
    // shift(saved_location) >> state_bank_;
  }
  RestoreState(ledger_home_location_, today, INT64_MAX, sync_schema);
  // for hidden pos && asset
  // shift(ledger_home_location_) >> state_bank_; // Load positions to restore bookkeeper
}

Watcher::~Watcher() {
  SPDLOG_INFO("~Watcher");
  uv_work_.data = nullptr;
  strategy_states_ref_.Reset();
  config_ref_.Reset();
  app_states_ref_.Reset();
  ledger_ref_.Reset();
  SPDLOG_INFO("~Watcher Done");
}

void Watcher::NoSet(const Napi::CallbackInfo &info, const Napi::Value &value) {
  SPDLOG_WARN("do not manipulate watcher internals");
}

Napi::Value Watcher::HasLocation(const Napi::CallbackInfo &info) {
  uint32_t uid = 0;
  if (info[0].IsNumber()) {
    uid = info[0].ToNumber().Uint32Value();
  }
  if (info[0].IsString()) {
    std::stringstream ss;
    ss << std::hex << info[0].ToString().Utf8Value();
    ss >> uid;
  }

  return Napi::Boolean::New(info.Env(), has_location(uid));
}

Napi::Value Watcher::GetLocation(const Napi::CallbackInfo &info) {
  auto location = FindLocation(info);
  if (not location) {
    return {};
  }
  auto locationObj = Napi::Object::New(info.Env());
  locationObj.Set("category", Napi::String::New(info.Env(), get_category_name(location->category)));
  locationObj.Set("group", Napi::String::New(info.Env(), location->group));
  locationObj.Set("name", Napi::String::New(info.Env(), location->name));
  locationObj.Set("mode", Napi::String::New(info.Env(), get_mode_name(location->mode)));
  locationObj.Set("uname", Napi::String::New(info.Env(), location->uname));
  locationObj.Set("uid", Napi::Number::New(info.Env(), location->uid));
  return locationObj;
}

Napi::Value Watcher::GetLocationUID(const Napi::CallbackInfo &info) {
  auto target_location = IODevice::ExtractLocation(info, 0, get_locator());
  return Napi::Number::New(info.Env(), target_location->uid);
}

// TODO: 返回十进制，但需要16进制
Napi::Value Watcher::GetInstrumentUID(const Napi::CallbackInfo &info) {
  auto exchange_id = info[0].ToString().Utf8Value();
  auto instrument_id = info[1].ToString().Utf8Value();
  auto key = hash_instrument(exchange_id.c_str(), instrument_id.c_str());
  return Napi::Number::New(info.Env(), key);
}

Napi::Value Watcher::GetInstrumentType(const Napi::CallbackInfo &info) {
  auto exchange_id = info[0].ToString().Utf8Value();
  auto instrument_id = info[1].ToString().Utf8Value();
  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  return Napi::Number::New(info.Env(), int(instrument_type));
}

Napi::Value Watcher::GetLedger(const Napi::CallbackInfo &info) { return ledger_ref_.Value(); }

Napi::Value Watcher::GetAppStates(const Napi::CallbackInfo &info) { return app_states_ref_.Value(); }

Napi::Value Watcher::GetStrategyStates(const Napi::CallbackInfo &info) { return strategy_states_ref_.Value(); }

Napi::Value Watcher::Now(const Napi::CallbackInfo &info) {
  return Napi::BigInt::New(ledger_ref_.Env(), time::now_in_nano());
}

Napi::Value Watcher::IsUsable(const Napi::CallbackInfo &info) { return Napi::Boolean::New(info.Env(), is_usable()); }

Napi::Value Watcher::IsLive(const Napi::CallbackInfo &info) { return Napi::Boolean::New(info.Env(), is_live()); }

Napi::Value Watcher::IsStarted(const Napi::CallbackInfo &info) { return Napi::Boolean::New(info.Env(), is_started()); }

Napi::Value Watcher::RequestPosition(const Napi::CallbackInfo &info) {
  if (not has_writer(ledger_home_location_->uid)) {
    return Napi::Boolean::New(info.Env(), false);
  }

  get_writer(ledger_home_location_->uid)->mark(now(), PositionRequest::tag);
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value Watcher::RequestStop(const Napi::CallbackInfo &info) {
  auto app_location = IODevice::ExtractLocation(info, 0, get_locator());

  // stop master
  if (app_location->category == category::SYSTEM && app_location->group == "master") {
    if (not has_writer(get_master_command_uid())) {
      return Napi::Boolean::New(info.Env(), false);
    }
    get_writer(get_master_command_uid())->mark(now(), RequestStop::tag);
    return Napi::Boolean::New(info.Env(), true);
  }

  if (not has_writer(app_location->uid)) {
    return Napi::Boolean::New(info.Env(), false);
  }
  get_writer(app_location->uid)->mark(now(), RequestStop::tag);
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value Watcher::PublishState(const Napi::CallbackInfo &info) {
  if (IsValid(info, 0, &Napi::Value::IsObject)) {
    publish(info[0].ToObject());
  }
  return {};
}

Napi::Value Watcher::IsReadyToInteract(const Napi::CallbackInfo &info) {
  auto account_location = IODevice::ExtractLocation(info, 0, get_locator());
  return Napi::Boolean::New(info.Env(), account_location and has_writer(account_location->uid) and
                                            is_location_live(account_location->uid));
}

Napi::Value Watcher::IssueCustomData(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue custom data manually");
  return InteractWithLocation<TimeKeyValue>(info, info[0].ToObject());
}

Napi::Value Watcher::IssueBlockMessage(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue block message manually");
  return InteractWithTD<BlockMessage>(info, info[0].ToObject(), &BlockMessage::block_id);
}

Napi::Value Watcher::IssueOrderTrigger(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue order trigger manually");
  return InteractWithTD<OrderTriggerInput>(info, info[0].ToObject(), &OrderTriggerInput::trigger_id);
}

Napi::Value Watcher::IssueOrder(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue order manually");
  return InteractWithTD<OrderInput>(info, info[0].ToObject(), &OrderInput::order_id);
}

Napi::Value Watcher::IssueAlgoOrder(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue algo order manually");
  return InteractWithTD<AlgoOrderInput>(info, info[0].ToObject(), &AlgoOrderInput::order_id);
}

Napi::Value Watcher::IssueMark(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("issue mark");
  uint32_t tag = GetNumber(info, 0);
  auto target_location = IODevice::ExtractLocation(info, 1, get_locator());
  if (not has_writer(target_location->location_uid)) {
    return Napi::Boolean::New(info.Env(), false);
  }
  get_writer(target_location->location_uid)->mark(time::now_in_nano(), tag);
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value Watcher::CancelOrder(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("cancel order manually");
  return InteractWithTD<OrderAction>(info, info[0].ToObject(), &OrderAction::order_action_id);
}

Napi::Value Watcher::CancelAlgoOrder(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("cancel algo order manually");
  return InteractWithTD<AlgoOrderAction>(info, info[0].ToObject(), &AlgoOrderAction::order_action_id);
}

Napi::Value Watcher::CancelOrderTrigger(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("cancel order trigger manually");
  return InteractWithTD<OrderTriggerAction>(info, info[0].ToObject(), &OrderTriggerAction::order_trigger_action_id);
}

Napi::Value Watcher::ToggleAlgoOrder(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("toggle algo order manually");
  return InteractWithTD<AlgoOrderAction>(info, info[0].ToObject(), &AlgoOrderAction::order_action_id);
}

Napi::Value Watcher::RequestMarketData(const Napi::CallbackInfo &info) {
  if (not IsValid(info, 0, &Napi::Value::IsObject)) {
    return Napi::Boolean::New(info.Env(), false);
  }

  if (not IsValid(info, 1, &Napi::Value::IsString)) {
    return Napi::Boolean::New(info.Env(), false);
  }

  if (not IsValid(info, 2, &Napi::Value::IsString)) {
    return Napi::Boolean::New(info.Env(), false);
  }

  auto md_location = IODevice::ExtractLocation(info, 0, get_locator());
  auto exchange_id = info[1].ToString().Utf8Value();
  auto instrument_id = info[2].ToString().Utf8Value();

  SPDLOG_INFO("request market data {}@{} from {}", exchange_id, instrument_id, md_location->uname);

  if (not has_writer(md_location->uid)) {
    return Napi::Boolean::New(info.Env(), false);
  }

  broker_client_.subscribe(md_location, exchange_id, instrument_id);
  auto writer = get_writer(md_location->uid);
  uint32_t key = hash_instrument(exchange_id.c_str(), instrument_id.c_str());
  InstrumentKey instrument_key = {};
  instrument_key.key = key;
  strcpy(instrument_key.instrument_id, instrument_id.c_str());
  strcpy(instrument_key.exchange_id, exchange_id.c_str());
  instrument_key.instrument_type = get_instrument_type(exchange_id, instrument_id);
  writer->write(now(), instrument_key);
  subscribed_instruments_.emplace(key, instrument_key);

  return Napi::Boolean::New(info.Env(), true);
}

void Watcher::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);
  env.AddCleanupHook(cleanup);

  Napi::Function func =
      DefineClass(env, "Watcher",
                  {
                      InstanceMethod("now", &Watcher::Now),                                             //
                      InstanceMethod("isUsable", &Watcher::IsUsable),                                   //
                      InstanceMethod("isLive", &Watcher::IsLive),                                       //
                      InstanceMethod("isStarted", &Watcher::IsStarted),                                 //
                      InstanceMethod("requestStop", &Watcher::RequestStop),                             //
                      InstanceMethod("hasLocation", &Watcher::HasLocation),                             //
                      InstanceMethod("getLocation", &Watcher::GetLocation),                             //
                      InstanceMethod("getLocationUID", &Watcher::GetLocationUID),                       //
                      InstanceMethod("getInstrumentType", &Watcher::GetInstrumentType),                 //
                      InstanceMethod("publishState", &Watcher::PublishState),                           //
                      InstanceMethod("isReadyToInteract", &Watcher::IsReadyToInteract),                 //
                      InstanceMethod("issueCustomData", &Watcher::IssueCustomData),                     //
                      InstanceMethod("issueBlockMessage", &Watcher::IssueBlockMessage),                 //
                      InstanceMethod("issueOrderTrigger", &Watcher::IssueOrderTrigger),                 //
                      InstanceMethod("issueOrder", &Watcher::IssueOrder),                               //
                      InstanceMethod("issueAlgoOrder", &Watcher::IssueAlgoOrder),                       //
                      InstanceMethod("issueMark", &Watcher::IssueMark),                                 //
                      InstanceMethod("cancelOrder", &Watcher::CancelOrder),                             //
                      InstanceMethod("cancelAlgoOrder", &Watcher::CancelAlgoOrder),                     //
                      InstanceMethod("cancelOrderTrigger", &Watcher::CancelOrderTrigger),               //
                      InstanceMethod("toggleAlgoOrder", &Watcher::ToggleAlgoOrder),                     //
                      InstanceMethod("requestMarketData", &Watcher::RequestMarketData),                 //
                      InstanceMethod("requestPosition", &Watcher::RequestPosition),                     //
                      InstanceMethod("start", &Watcher::Start),                                         //
                      InstanceMethod("sync", &Watcher::Sync),                                           //
                      InstanceMethod("quit", &Watcher::Quit),                                           //
                      InstanceAccessor("ledger", &Watcher::GetLedger, &Watcher::NoSet),                 //
                      InstanceAccessor("appStates", &Watcher::GetAppStates, &Watcher::NoSet),           //
                      InstanceAccessor("strategyStates", &Watcher::GetStrategyStates, &Watcher::NoSet), //
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Watcher", func);
}

void Watcher::on_react() {
  SPDLOG_INFO("watcher on react");

  events_ | is(Register::tag) | $$(OnRegister(event->gen_time(), event->data<Register>()));
  events_ | is(Deregister::tag) | $$(OnDeregister(event->gen_time(), event->data<Deregister>()));
  // for receive history data
  auto before_start_events = events_ | take_until(events_ | is(RequestStart::tag));
  // accept trading data from cached state, so even if ui reload, history data is able to be shown
  before_start_events | is_trading_data() | skip_while(while_is(OrderInput::tag)) |
      $$(cached::feed_state_data(event, trading_data_cached_bank_));
  before_start_events | not_trading_data() | $$(cached::feed_state_data(event, data_bank_));
}

bool Watcher::has_writer(uint32_t dest_id) const { return writers_.find(dest_id) != writers_.end(); }

journal::writer_ptr Watcher::get_writer(uint32_t dest_id) const {
  if (writers_.find(dest_id) == writers_.end()) {
    SPDLOG_ERROR("no writer for {}", get_location_uname(dest_id));
  }
  return writers_.at(dest_id);
}

void Watcher::on_start() {
  broker_client_.on_start(events_);

  if (not bypass_trading_data_) {
    bookkeeper_.on_start(events_);
    bookkeeper_.guard_positions();
    bookkeeper_.add_book_listener(std::make_shared<BookListener>(*this));

    // position should be always read from bookkeeper in watcher, because of position_guard, instead of feeds;
    events_ | not_trading_data() | skip_while(while_is(Position::tag)) | $$(cached::feed_state_data(event, data_bank_));

    // for reduce cpu consuming, if in bypass_accounting mode, the bookkeeper no longer update position/asset by trading
    // data, all relay on poistion updating by ledger;
    if (not bypass_accounting_) {
      events_ | is(Quote::tag) | is_subscribed(subscribed_instruments_) | $$(UpdateBook(event, event->data<Quote>()));
    }

    events_ | is(Order::tag) | $$(UpdateBook(event, event->data<Order>()));
    events_ | is(Trade::tag) | $$(UpdateBook(event, event->data<Trade>()));
    events_ | is(Asset::tag) | $$(UpdateAsset(event, event->data<Asset>().holder_uid));
    events_ | is(Position::tag) | $$(UpdateBook(event, event->data<Position>()));
    events_ | is(PositionEnd::tag) | $$(UpdateAsset(event, event->data<PositionEnd>().holder_uid));

    if (not bypass_refresh_book_) {
      refresh_books();
    }
  }

  events_ | is(Channel::tag) | $$(InspectChannel(event->gen_time(), event->data<Channel>()));
  events_ | is(BrokerStateUpdate::tag) |
      $$(UpdateBrokerOperatorState<BrokerStateUpdate>(event->source(), event->dest(),
                                                      event->data<BrokerStateUpdate>()));
  events_ | is(OperatorStateUpdate::tag) |
      $$(UpdateBrokerOperatorState<OperatorStateUpdate>(event->source(), event->dest(),
                                                        event->data<OperatorStateUpdate>()));
  events_ | is(StrategyStateUpdate::tag) | $$(UpdateStrategyState(event->source(), event->data<StrategyStateUpdate>()));
  events_ | is(CacheReset::tag) | $$(UpdateEventCache(event));
}

void Watcher::refresh_books() {
  for (const auto &pair : bookkeeper_.get_books()) {
    if (pair.second->asset.ledger_category == LedgerCategory::Account) {
      refresh_account_book(now(), pair.first);
    }
  }
}

void Watcher::refresh_account_book(int64_t trigger_time, uint32_t account_uid) {
  if (bypass_refresh_book_) {
    return;
  }
  auto account_location = get_location(account_uid);
  auto group = account_location->group;
  auto md_location = location::make_shared(account_location->mode, category::MD, group, group, get_locator());
  auto book = bookkeeper_.get_book(account_uid);
  auto subscribe_positions = [&](auto positions) {
    for (const auto &pair : positions) {
      auto &position = pair.second;
      broker_client_.subscribe(md_location, position.exchange_id, position.instrument_id);
    }
  };

  subscribe_positions(book->long_positions);
  subscribe_positions(book->short_positions);
}

void Watcher::RestoreState(const location_ptr &state_location, int64_t from, int64_t to, bool sync_schema) {
  add_location(0, state_location);
  // serialize::JsRestoreState(ledger_ref_, state_location)(from, to, sync_schema);
  // for hidden pos && asset
  serialize::JsRestoreState(ledger_ref_, state_location).filter_no<Position, Asset, Contract>(from, to, sync_schema);
}

Napi::Value Watcher::Start(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("start");
  StartWorker();
  return {};
}

void Watcher::Sync(const Napi::CallbackInfo &info) {
  std::lock_guard<std::mutex> guard(feed_mutex_);
  SyncEventCache();
  SyncAppStates();
  SyncStrategyStates();
  SyncLedger();
  TryRefreshTradingData();
  SyncTradingDataFromCached();
  SyncTradingData();
  ResetTradingDataCount();
}

void Watcher::SyncLedger() {
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    // cause trading data saved in trading_data_bank, no need to filter at this point
    UpdateLedger(+boost::hana::second(it));
  });
}

void Watcher::TryRefreshTradingData() {
  if (not refresh_trading_data_before_sync_) {
    return;
  }

  serialize::RefreshTradingDataInStateMap(ledger_ref_, "ledger");
}

void Watcher::SyncTradingData() {
  boost::hana::for_each(longfist::RefreshRequiredDataTypes,
                        [&](auto it) { UpdateTradingData(+boost::hana::second(it)); });
}

void Watcher::SyncTradingDataFromCached() {
  boost::hana::for_each(longfist::RefreshRequiredDataTypes,
                        [&](auto it) { UpdateTradingDataFromCacheD(+boost::hana::second(it)); });
}

void Watcher::SyncAppStates() {
  for (auto &s : broker_states_map_) {
    auto app_state = Napi::Number::New(app_states_ref_.Env(), s.second);
    app_states_ref_.Set(format(s.first), app_state);
  }
  broker_states_map_.clear();
}

void Watcher::SyncStrategyStates() {
  for (auto &s : strategy_states_map_) {
    auto strategy_state_obj = Napi::Object::New(strategy_states_ref_.Env());
    strategy_state_obj.Set("state", Napi::Number::New(strategy_states_ref_.Env(), int(s.second.state)));
    strategy_state_obj.Set("update_time", Napi::Number::New(strategy_states_ref_.Env(), s.second.update_time));
    strategy_state_obj.Set("info_a", Napi::String::New(strategy_states_ref_.Env(), s.second.info_a));
    strategy_state_obj.Set("info_b", Napi::String::New(strategy_states_ref_.Env(), s.second.info_b));
    strategy_state_obj.Set("info_c", Napi::String::New(strategy_states_ref_.Env(), s.second.info_c));
    strategy_state_obj.Set("value", Napi::String::New(strategy_states_ref_.Env(), s.second.value));
    strategy_states_ref_.Set(format(s.first), strategy_state_obj);
  }
  strategy_states_map_.clear();
}

void Watcher::SyncEventCache() {
  if (reset_cache_states_.size()) {
    for (const auto &reset_state : reset_cache_states_) {
      reset_cache(reset_state);
    }
    reset_cache_states_.clear();
  }
}

void Watcher::UpdateEventCache(const event_ptr &event) {
  const auto &request = event->data<CacheReset>();
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (DataType::tag == request.msg_type) {
      auto hana_type = boost::hana::type_c<DataType>;
      using DelMap = std::unordered_map<uint64_t, state<DataType>>;
      auto &del_map = const_cast<DelMap &>(data_bank_[hana_type]);
      auto iter = del_map.begin();
      while (iter != del_map.end()) {
        auto s = iter->second;
        auto source_id = s.source;
        auto dest_id = s.dest;
        if ((source_id == event->source() and dest_id == event->dest()) || source_id == event->dest()) {
          iter = del_map.erase(iter);
        } else {
          iter++;
        }
      }
    }
  });
  reset_cache_states_.push_back(state<CacheReset>(event));
}

location_ptr Watcher::FindLocation(const Napi::CallbackInfo &info) {
  if (info.Length() == 0) {
    return get_io_device()->get_live_home();
  }
  uint32_t uid = 0;
  if (info[0].IsNumber()) {
    uid = info[0].ToNumber().Uint32Value();
  }
  if (info[0].IsString()) {
    std::stringstream ss;
    ss << std::hex << info[0].ToString().Utf8Value();
    ss >> uid;
  }
  if (has_location(uid)) {
    return get_location(uid);
  }
  return location_ptr();
}

void Watcher::InspectChannel(int64_t trigger_time, const Channel &channel) {
  if (bypass_trading_data_) {
    return;
  }

  if (channel.source_id != get_live_home_uid() and channel.dest_id != get_live_home_uid()) {
    reader_join(channel.source_id, channel.dest_id, trigger_time);
  }

  if (has_location(channel.source_id) && has_location(channel.dest_id)) {
    auto source_location = get_location(channel.source_id);
    auto dest_location = get_location(channel.dest_id);

    if (source_location->category == category::TD) {
      if (dest_location->group == "node") { // for as soon as possible to get trading data made by renderer process;
        trading_data_reader_->join(source_location, channel.dest_id, get_begin_time(), 0, Priority::High);
      } else {
        trading_data_reader_->join(source_location, channel.dest_id, get_begin_time());
      }
    }

    // for order stat
    if (source_location->uid == ledger_home_location_->uid) {
      trading_data_reader_->join(source_location, channel.dest_id, get_begin_time());
    }
  }
}

void Watcher::MonitorMarketData(int64_t trigger_time, const location_ptr &md_location) {
  //  events_ | is(Quote::tag) | from(md_location->uid) | first() |
  //      $(
  //          [&, trigger_time, md_location](const event_ptr &event) {
  //            broker_states_map_.insert_or_assign(md_location->uid, int(BrokerState::Ready));
  //            events_ | from(md_location->uid) | is(Quote::tag) |
  //                timeout(std::chrono::seconds(15), get_timer_usage_count()) |
  //                $(noop_event_handler(), [&, trigger_time, md_location](std::exception_ptr e) {
  //                  if (is_location_live(md_location->uid)) {
  //                    broker_states_map_.insert_or_assign(md_location->uid, int(BrokerState::Idle));
  //                    MonitorMarketData(trigger_time, md_location);
  //                  }
  //                });
  //          },
  //          error_handler_log(fmt::format("monitor md {}", md_location->uname)));
}

void Watcher::OnRegister(int64_t trigger_time, const Register &register_data) {
  auto app_uid = register_data.location_uid;
  if (app_uid == get_live_home_uid()) {
    return;
  }

  auto app_location = get_location(app_uid);
  if (app_location->category == category::MD or app_location->category == category::TD or
      app_location->category == category::OPERATOR) {
    broker_states_map_.insert_or_assign(app_location->uid, int(BrokerState::Pending));
  }

  if (app_location->category == category::MD and app_location->mode == mode::LIVE) {
    MonitorMarketData(trigger_time, app_location);
  }

  if (app_location->category == category::TD) {
    trading_data_reader_->join(app_location, location::PUBLIC, get_begin_time());
  }
}

void Watcher::OnDeregister(int64_t trigger_time, const Deregister &deregister_data) {
  auto app_location = location::make_shared(deregister_data, get_locator());
  if (app_location->category == category::MD or app_location->category == category::TD or
      app_location->category == category::OPERATOR) {
    broker_states_map_.insert_or_assign(app_location->uid, int(BrokerState::Pending));
  }

  if (app_location->category == category::SYSTEM and app_location->group == "master" and
      app_location->name == "master") {
    CancelWorker();
  }
}

void Watcher::StartWorker() {
  uv_work_.data = static_cast<void *>(this);
  uv_work_live_ = true;
  auto worker = [](uv_work_t *req) {
    auto watcher = static_cast<Watcher *>(req->data);
    while (req->data && watcher->uv_work_live_) {

      if (not watcher->is_live() and not watcher->is_started() and watcher->is_usable()) {
        watcher->setup();
      }
      while (watcher->is_live()) {
        std::lock_guard<std::mutex> guard(watcher->feed_mutex_);

        if (not watcher->is_step_continually()) {
          break;
        }

        watcher->step(STEP_INTERVAL);
        watcher->drain_from_trading_data_reader(STEP_INTERVAL);
      }
      std::this_thread::sleep_for(std::chrono::microseconds(watcher->milliseconds_sleep_after_step_));
    }
    watcher->signal_stop();
    watcher->pause();
    SPDLOG_INFO("Watcher uv loop stopped");
  };
  auto after = [](uv_work_t *req, int status) {
    SPDLOG_INFO("Watcher uv loop completed");
    // have to be at this position, for deleting old journal securitily
    auto &info = *static_cast<Napi::CallbackInfo *>(req->data);
    Napi::HandleScope scope(info.Env());

    auto watcher = static_cast<Watcher *>(req->data);
    if (watcher->quit_) {
      SPDLOG_INFO("watcher quit");
      return;
    } else {
      // have to wait for master down totally
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    watcher->AfterMasterDown(info);
    watcher->set_begin_time(time::now_in_nano());
    SPDLOG_INFO("Restart watcher uv loop");
    // master may quit within watcher running time,
    // so, once master deregistered, the uv logic in watcher need to be restarte.
    watcher->StartWorker();
  };

  uv_queue_work(uv_default_loop(), &uv_work_, worker, after);
}

void Watcher::CancelWorker() { uv_work_live_ = false; }

void Watcher::Quit(const Napi::CallbackInfo &info) {
  RequestDeregister();
  quit_ = true;
  uv_work_live_ = false;
}

void Watcher::RequestDeregister() {
  if (not has_writer(get_master_command_uid())) {
    SPDLOG_WARN("no master cmd writer");
    return;
  }

  auto writer = get_writer(get_master_command_uid());
  writer->mark(now(), RequestDeregister::tag);
  SPDLOG_INFO("RequestDeregister");
}

void Watcher::AfterMasterDown(const Napi::CallbackInfo &info) {
  SPDLOG_INFO("after master down");
  Napi::HandleScope scope(info.Env());
  //  disjoin(get_master_command_uid());
  reader_->clear();
  trading_data_reader_->clear();
  writers_.clear();
  band_writers_.clear();
  serialize::InitObjectReference(info, app_states_ref_);
  serialize::InitObjectReference(info, strategy_states_ref_);
  serialize::InitStateMap(info, ledger_ref_, "ledger");
}

void Watcher::UpdateStrategyState(uint32_t strategy_uid, const StrategyStateUpdate &state) {
  auto app_location = get_location(strategy_uid);
  strategy_states_map_.insert_or_assign(app_location->uid, state);
}

void Watcher::UpdateAsset(const event_ptr &event, uint32_t book_uid) {
  auto book = bookkeeper_.get_book(book_uid);
  state<Asset> cache_state_asset(ledger_home_location_->uid, book_uid, event->gen_time(), book->asset);
  feed_state_data_bank(cache_state_asset, data_bank_);
}

void Watcher::UpdateBook(const event_ptr &event, const Quote &quote) {
  auto ledger_uid = ledger_home_location_->uid;
  for (const auto &item : bookkeeper_.get_books()) {
    auto &book = item.second;
    auto holder_uid = book->asset.holder_uid;

    if (holder_uid == ledger_uid) {
      continue;
    }

    auto apply = [&](auto &position) { UpdateBook(event, position); };
    book->apply_long_position_for(quote, apply);
    book->apply_short_position_for(quote, apply);

    state<Asset> cache_state_asset(ledger_uid, holder_uid, event->gen_time(), book->asset);
    feed_state_data_bank(cache_state_asset, data_bank_);
  }
}

bool Watcher::is_reactable(const event_ptr &event) {
  if (event->msg_type() == Transaction::tag or event->msg_type() == Entrust::tag or event->msg_type() == Tree::tag or
      event->msg_type() == Tick::tag or event->msg_type() == Depth::tag) {
    return false;
  }

  // is_started() is supposed to be accpeted RefreshRequiredDataTags from cached
  if ((bypass_trading_data_ or (bypass_accounting_ and is_started())) and
      longfist::RefreshRequiredDataTags.find(event->msg_type()) != longfist::RefreshRequiredDataTags.end()) {
    return false;
  }

  auto iter = broker::map_is_own_event.find(event->msg_type());
  if (iter != broker::map_is_own_event.end()) {
    return iter->second(broker_client_, event);
  }

  return not is_custom_event(event);
}

void Watcher::drain_from_trading_data_reader(uint32_t step_limit) {
  std::size_t step_count = 0;
  while ((step_limit == 0 || step_count < step_limit) and
         trading_data_count_by_step_ < REFRESH_REQUIRED_DATA_LIMIT_BY_SYNC and trading_data_reader_->data_available()) {
    const yijinjing::journal::frame_ptr frame = trading_data_reader_->current_frame();

    if (longfist::RefreshRequiredDataTags.find(frame->msg_type()) != longfist::RefreshRequiredDataTags.end() and
        frame->msg_type() != OrderInput::tag) {
      cached::feed_state_data(frame, trading_data_bank_);
      trading_data_count_by_step_++;
    }
    step_count++;
    trading_data_reader_->next();
  }
}

bool Watcher::is_step_continually() {
  bool default_reader_available = reader_->data_available();
  bool trading_data_reader_available = trading_data_reader_->data_available();
  bool is_read_enough = trading_data_count_by_step_ >= REFRESH_REQUIRED_DATA_LIMIT_BY_SYNC;
  return default_reader_available || (trading_data_reader_available && not is_read_enough);
}

void Watcher::UpdateBook(const event_ptr &event, const Position &position) {
  auto book = bookkeeper_.get_book(position.holder_uid);

  auto apply = [&](auto &position) {
    state<Position> cache_state_position(position.holder_uid, event->dest(), event->gen_time(), position);
    feed_state_data_bank(cache_state_position, data_bank_);
  };

  book->apply_position(position.source_id, position.direction, position.exchange_id, position.instrument_id, apply);
  book->apply_opposite_position(position.source_id, position.direction, position.exchange_id, position.instrument_id,
                                apply);
}

Watcher::BookListener::BookListener(Watcher &watcher) : watcher_(watcher) {}

void Watcher::BookListener::on_asset_sync_reset(const Asset &old_asset, const Asset &new_asset) {
  state<Asset> cache_state(watcher_.ledger_home_location_->uid, new_asset.holder_uid, new_asset.update_time, new_asset);
  watcher_.feed_state_data_bank(cache_state, watcher_.data_bank_);
}

void Watcher::BookListener::on_position_sync_reset(const book::Book &old_book, const book::Book &new_book) {
  auto update_position = [&](const auto &position) {
    state<Position> cache_state(watcher_.ledger_home_location_->uid, position.holder_uid, position.update_time,
                                position);
    watcher_.feed_state_data_bank(cache_state, watcher_.data_bank_);
  };

  const_cast<book::Book &>(new_book).apply_long_positions(update_position);
  const_cast<book::Book &>(new_book).apply_short_positions(update_position);
}

} // namespace kungfu::node