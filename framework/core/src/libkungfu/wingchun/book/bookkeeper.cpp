// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/4/6.
//

#include <kungfu/wingchun/book/bookkeeper.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::broker;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::book {
Bookkeeper::Bookkeeper(apprentice &app, broker::Client &broker_client, bool bypass_quote)
    : app_(app), broker_client_(broker_client), bypass_quote_(bypass_quote),
      account_method_type_(book::get_accounting_method_type()) {
  book::AccountingMethod::setup_defaults(*this, account_method_type_);
}

bool Bookkeeper::has_book(uint32_t location_uid) { return books_.find(location_uid) != books_.end(); }

void Bookkeeper::drop_book(uint32_t uid) { books_.erase(uid); }

Book_ptr Bookkeeper::get_book(uint32_t location_uid) {
  if (books_.find(location_uid) == books_.end()) {
    books_.emplace(location_uid, make_book(location_uid));
  }
  return books_.at(location_uid);
}

const BookMap &Bookkeeper::get_books() const { return books_; }

void Bookkeeper::set_accounting_method(InstrumentType instrument_type, const AccountingMethod_ptr &accounting_method) {
  accounting_methods_.emplace(instrument_type, accounting_method);
}

void Bookkeeper::on_start(const rx::connectable_observable<event_ptr> &events) {
  restore(app_.get_state_bank());

  events | is(Instrument::tag) | $$(update_instrument(event->data<Instrument>()));
  events | is(Commission::tag) | $$(update_commission(event, event->data<Commission>()));
  events | is(InstrumentFactor::tag) | $$(update_instrument_factor(event->data<InstrumentFactor>()));
  events | is_own<Quote>(broker_client_) | $$(try_update_book(event, event->data<Quote>()));
  events | is(InstrumentKey::tag) | $$(update_book(event, event->data<InstrumentKey>()));
  events | is(OrderInput::tag) |
      $$(on_order_input(event->gen_time(), event->source(), event->dest(), event->data<OrderInput>()));
  events | is(Order::tag) | $$(update_book<Order>(event, &AccountingMethod::apply_order));
  events | is(Trade::tag) | $$(update_book<Trade>(event, &AccountingMethod::apply_trade));
  events | fork<Asset>(location::SYNC, &Bookkeeper::try_update_asset_replica, &Bookkeeper::try_update_asset);
  events | fork<Position>(location::SYNC, &Bookkeeper::try_update_position_replica, &Bookkeeper::try_update_position);
  events | fork<PositionEnd>(location::SYNC, &Bookkeeper::update_position_guard, &Bookkeeper::try_update_position_end);
  events | is(ResetBookRequest::tag) | $$(drop_book(event->source()));
  events | is(OutputKey::tag) | $$(on_output_key(event));

  if (bypass_quote_) {
    app_.add_time_interval(yijinjing::time_unit::NANOSECONDS_PER_SECOND * 15,
                           [&](auto e) { batch_update_book_by_quote(); });
  }
}

void Bookkeeper::batch_update_book_by_quote() {
  SPDLOG_DEBUG("batch_update_book_by_quote");

  for (const auto &iter : quotes_) {
    const auto &state_quote = iter.second;
    update_book(state_quote.update_time, state_quote.data);
  }
  quotes_.clear();
}

std::mutex &Bookkeeper::get_update_book_mutex() { return update_book_mutex_; }

void Bookkeeper::try_update_position_end(const PositionEnd &position_end) {
  get_book(position_end.holder_uid)->update(app_.now(), account_method_type_);
}

void Bookkeeper::on_order_input(int64_t update_time, uint32_t source, uint32_t dest, const OrderInput &input) {
  update_book<OrderInput>(update_time, dest, source, input, &AccountingMethod::apply_order_input);
}

void Bookkeeper::restore(const cache::bank &state_bank) {
  for (auto &pair : state_bank[boost::hana::type_c<Instrument>]) {
    update_instrument(pair.second.data);
  }
  for (auto &pair : state_bank[boost::hana::type_c<Commission>]) {
    auto &state = pair.second;
    auto &commission = state.data;
    commissions_.insert_or_assign(hash_str_32(commission.product_id), commission);
  }
  for (auto &pair : state_bank[boost::hana::type_c<Position>]) {
    auto &state = pair.second;
    auto &position = state.data;
    if (not app_.has_location(position.holder_uid) or not app_.has_location(position.source_id)) {
      continue;
    }
    auto book = get_book(position.holder_uid);
    auto is_long = position.direction == longfist::enums::Direction::Long;
    auto &positions = is_long ? book->long_positions : book->short_positions;
    positions[hash_instrument(position.source_id, position.exchange_id, position.instrument_id)] = position;
    positions[hash_instrument(position.source_id, position.exchange_id, position.instrument_id)].source_op_id =
        book->source_op_id(position.holder_uid, position.source_id);
    book->add_source_id(position.source_id);
  }
  for (auto &pair : state_bank[boost::hana::type_c<Asset>]) {
    auto &state = pair.second;
    auto &asset = state.data;
    if (not app_.has_location(asset.holder_uid)) {
      continue;
    }
    auto book = get_book(asset.holder_uid);
    book->asset = asset;
    book->update(app_.now(), account_method_type_);
  }

  for (auto &pair : state_bank[boost::hana::type_c<InstrumentFactor>]) {
    auto &state = pair.second;
    auto &instrument_factor = state.data;
    if (not app_.has_location(instrument_factor.source_id)) {
      continue;
    }
    update_instrument_factor(instrument_factor);
  }
}

void Bookkeeper::guard_positions() { positions_guarded_ = true; }

Book_ptr Bookkeeper::make_book(uint32_t location_uid) {
  auto location = app_.get_location(location_uid);
  auto book = std::make_shared<Book>(commissions_, instruments_);
  auto &asset = book->asset;
  asset.holder_uid = location_uid;
  asset.ledger_category = location->category == category::TD ? LedgerCategory::Account : LedgerCategory::Strategy;
  return book;
}

void Bookkeeper::update_instrument(const longfist::types::Instrument &instrument) {
  auto hashed_instrument_key = hash_instrument(instrument.exchange_id, instrument.instrument_id);
  instruments_.insert_or_assign(hashed_instrument_key, instrument);
}

void Bookkeeper::update_commission(const event_ptr &event, const longfist::types::Commission &commission) {
  for (auto &bk_pair : books_) {
    auto &book = bk_pair.second;
    if (book->asset.holder_uid == event->source()) {
      book->replace(commission);
    }
  }
}

void Bookkeeper::update_instrument_factor(const longfist::types::InstrumentFactor &instrument_factor) {
  for (auto &bk_pair : books_) {
    auto &book = bk_pair.second;
    auto location = app_.get_location(book->asset.holder_uid);
    if (location->category != category::TD or book->asset.holder_uid == instrument_factor.source_id) {
      book->replace(instrument_factor);
    }
  }
}

void Bookkeeper::update_book(const event_ptr &event, const InstrumentKey &instrument_key) {
  std::lock_guard<std::mutex> lock(update_book_mutex_);
  broker_client_.subscribe(instrument_key);
  get_book(event->source())->ensure_position_for(instrument_key);
}

void Bookkeeper::try_update_book(const event_ptr &event, const Quote &quote) {
  if (bypass_quote_) {
    state<Quote> state_quote(event->source(), event->dest(), event->gen_time(), quote);
    auto hashed_instrument_key = hash_instrument(quote.exchange_id, quote.instrument_id);
    quotes_.insert_or_assign(hashed_instrument_key, state_quote);
    return;
  }

  update_book(event->gen_time(), quote);
}

void Bookkeeper::update_book(int64_t trigger_time, const Quote &quote) {
  std::lock_guard<std::mutex> lock(update_book_mutex_);
  if (accounting_methods_.find(quote.instrument_type) == accounting_methods_.end()) {
    return;
  }
  auto accounting_method = accounting_methods_.at(quote.instrument_type);
  auto apply = [&](auto &position) { position.update_time = trigger_time; };

  for (auto &item : books_) {
    auto &book = item.second;
    if (book->has_short_position_for(quote) or book->has_long_position_for(quote)) {
      accounting_method->apply_quote(book, quote);
      book->update(trigger_time, account_method_type_);
    }
    book->apply_long_position_for(quote, apply);
    book->apply_short_position_for(quote, apply);
  }
}

void Bookkeeper::try_update_asset(const Asset &asset) {
  if (app_.has_location(asset.holder_uid)) {
    get_book(asset.holder_uid)->asset = asset;
  }
}

void Bookkeeper::try_update_position(const Position &position) {
  if (not app_.has_location(position.holder_uid)) {
    return;
  }

  auto book = get_book(position.holder_uid);
  auto apply = [&](auto &target_position) {
    if (positions_guarded_ and target_position.update_time >= position.update_time) {
      return;
    }
    auto last_price = std::max(position.last_price, target_position.last_price);
    target_position = position;
    target_position.last_price = last_price;
    if (accounting_methods_.find(target_position.instrument_type) == accounting_methods_.end()) {
      return;
    }
    accounting_methods_.at(target_position.instrument_type)->update_position(book, target_position);
  };

  book->apply_position(position.source_id, position.direction, position.exchange_id, position.instrument_id, apply);
}

void Bookkeeper::try_sync_book_replica(uint32_t location_uid) {
  /// sync的Asset, AssetMargin, PositionEnd都收到后才开始同步TD和策略的信息, 并使用TD的新book替换旧book
  if (not books_replica_asset_guards_.try_emplace(location_uid).first->second or
      not books_replica_position_guard_.try_emplace(location_uid).first->second or
      not books_replica_asset_margin_guards_.try_emplace(location_uid).first->second) {
    return;
  }

  books_replica_asset_guards_.insert_or_assign(location_uid, false);
  books_replica_asset_margin_guards_.insert_or_assign(location_uid, false);
  books_replica_position_guard_.insert_or_assign(location_uid, false);
  auto old_book = get_book(location_uid);
  auto new_book = get_book_replica(location_uid);

  bool position_changed = false;
  bool asset_changed = false;


  // TODO: asset margin related compare
  auto asset_compare = [](const Asset &old_asset, const Asset &new_asset) {
    bool changed = false;
    changed |= old_asset.avail != new_asset.avail;   // 可用资金
    changed |= old_asset.margin != new_asset.margin; // 保证金(期货)
    return changed;
  };
  asset_changed |= asset_compare(old_book->asset, new_book->asset);

  auto position_compare = [](const PositionMap &position_map, Book_ptr &target_book) {
    bool changed = false;
    for (auto &source_pair : position_map) {
      auto &position = source_pair.second;
      auto &target_position = target_book->get_position(position.source_id, position.direction, position.exchange_id,
                                                        position.instrument_id);
      changed |= position.volume != target_position.volume;                     // 数量
      changed |= position.yesterday_volume != target_position.yesterday_volume; // 昨仓数量
    }
    return changed;
  };

  /// 用new_book的position去检测old_book的position,new有old无会加上
  position_changed |= position_compare(new_book->long_positions, old_book);
  position_changed |= position_compare(new_book->short_positions, old_book);
  /// 用old_book的position去检测new_book的position，old有new无会设置为0删掉
  position_changed |= position_compare(old_book->long_positions, new_book);
  position_changed |= position_compare(old_book->short_positions, new_book);

  /// position_changed更新book也会修改asset信息, on_asset_sync_reset仅在asset改变而position不改变的情况下调用
  if (asset_changed and not position_changed) {
    Asset old_asset = {};
    longfist::copy(old_asset, old_book->asset);       // old_asset拷贝一份用于回调返回
    longfist::copy(old_book->asset, new_book->asset); // new_asset替换掉old_asset

    for (auto &book_listener : book_listeners_) {
      book_listener->on_asset_sync_reset(old_asset, new_book->asset);
    }
  }

  /// position改变更新book，包括asset和position都更新并回调on_position_sync_reset
  if (position_changed) {
    /// 先进行td_book的替换, 否则下面mirror_position还是只会拿到旧的数据
    books_.erase(location_uid);
    books_.insert_or_assign(location_uid, new_book);

    /// 删除了watcher重新计算一遍的逻辑, 这里更新完了以后再调用回调, watcher那边获得的数据是更新完之后的
    for (auto &book_listener : book_listeners_) {
      book_listener->on_position_sync_reset(*old_book, *new_book);
    }
  }
  books_replica_.erase(location_uid); // delete replica every time
}

void Bookkeeper::try_update_asset_replica(const longfist::types::Asset &asset) {
  if (app_.has_location(asset.holder_uid)) {
    get_book_replica(asset.holder_uid)->asset = asset;
    books_replica_asset_guards_.insert_or_assign(asset.holder_uid, true);
    try_sync_book_replica(asset.holder_uid);
  }
}

void Bookkeeper::try_update_position_replica(const longfist::types::Position &position) {
  if (not app_.has_location(position.holder_uid)) {
    return;
  }
  auto book = get_book_replica(position.holder_uid);
  auto apply = [&](auto &target_position) { target_position = position; };
  book->apply_position(position.source_id, position.direction, position.exchange_id, position.instrument_id, apply);
}

Book_ptr Bookkeeper::get_book_replica(uint32_t location_uid) {
  if (books_replica_.find(location_uid) == books_replica_.end()) {
    books_replica_.emplace(location_uid, make_book(location_uid));
  }
  return books_replica_.at(location_uid);
}

void Bookkeeper::update_position_guard(const PositionEnd &position_end) {
  books_replica_position_guard_.insert_or_assign(position_end.holder_uid, true);
  try_sync_book_replica(position_end.holder_uid);
}

void Bookkeeper::add_book_listener(const BookListener_ptr &book_listener) { book_listeners_.push_back(book_listener); }

void Bookkeeper::mirror_positions(int64_t trigger_time, uint32_t strategy_uid) {
  auto strategy_book = get_book(strategy_uid);
  auto reset_positions = [trigger_time](auto &position) {
    position.volume = 0;
    position.yesterday_volume = 0;
    position.frozen_total = 0;
    position.frozen_yesterday = 0;
    position.avg_open_price = 0;
    position.position_cost_price = 0;
    position.update_time = trigger_time;
  };
  strategy_book->apply_short_positions(reset_positions);
  strategy_book->apply_long_positions(reset_positions);

  auto copy_positions = [&](auto &position) {
    if (strategy_book->has_position(position.source_id, position.direction, position.exchange_id,
                                    position.instrument_id)) {
      auto &strategy_position = strategy_book->get_position(position.source_id, position.direction,
                                                            position.exchange_id, position.instrument_id);
      longfist::copy(strategy_position, position);
      strategy_position.holder_uid = strategy_uid;
      strategy_position.ledger_category = LedgerCategory::Strategy;
      strategy_position.update_time = trigger_time;
      strategy_position.source_id = position.source_id;
    }
  };

  for (const auto &pair : get_books()) {
    auto &book = pair.second;
    auto holder_uid = book->asset.holder_uid;
    if (book->asset.ledger_category == LedgerCategory::Account and app_.has_channel(strategy_uid, holder_uid)) {
      book->apply_long_positions(copy_positions);
      book->apply_short_positions(copy_positions);
    }
  }
  strategy_book->update(trigger_time, account_method_type_);
}

void Bookkeeper::on_output_key(const event_ptr &event) {
  const OutputKey &key = event->data<OutputKey>();
  get_book(event->source())->add_source_id(key.location_uid);
}

} // namespace kungfu::wingchun::book