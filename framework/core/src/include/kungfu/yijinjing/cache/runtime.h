// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/3/27.
//

#ifndef KUNGFU_CACHE_RUNTIME_H
#define KUNGFU_CACHE_RUNTIME_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::yijinjing::cache {
class bank {
public:
  template <typename DataType> void operator<<(const state<DataType> &state) {
    auto &target_map = state_map_[boost::hana::type_c<DataType>];
    target_map.insert_or_assign(state.data.uid(), state);
  }

  template <typename DataType> void operator<<(const typed_event_ptr<DataType> &event) {
    auto &target_map = state_map_[boost::hana::type_c<DataType>];
    target_map.insert_or_assign(event->template data<DataType>().uid(), *event);
  }

  void operator>>(const yijinjing::journal::writer_ptr &writer) const {
    boost::hana::for_each(longfist::StateDataTypes, [&](auto it) {
      auto type = boost::hana::second(it);
      for (const auto &element : state_map_[type]) {
        writer->write(0, element.second.data);
      }
    });
  }

  void operator>>(yijinjing::cache::bank &bank) {
    boost::hana::for_each(longfist::StateDataTypes, [&](auto it) {
      auto type = boost::hana::second(it);
      for (const auto &element : state_map_[type]) {
        bank << element.second;
      }
    });
  }

  template <typename DataType>
  const std::unordered_map<uint64_t, state<DataType>> &operator[](const boost::hana::basic_type<DataType> &type) const {
    return state_map_[type];
  }

private:
  longfist::StateMapType state_map_ = longfist::build_state_map(longfist::StateDataTypes);
};

class location_bank {
public:
  template <typename DataType> void operator<<(const state<DataType> &state) {
    uint64_t source_to_dest = ((uint64_t)state.source << 32u) | state.dest;

    auto pair = location_state_map_.try_emplace(source_to_dest, longfist::build_state_map(longfist::StateDataTypes));
    auto &target_map = pair.first->second[boost::hana::type_c<DataType>];
    target_map.insert_or_assign(state.data.uid(), state);
  }

  const std::unordered_map<uint64_t, longfist::StateMapType> &get_map() const { return location_state_map_; }

private:
  std::unordered_map<uint64_t, longfist::StateMapType> location_state_map_ = {};
};

template <typename DataTypes, typename DataTypesMap> class typed_bank {
public:
  explicit typed_bank(const DataTypes &types) : types_(types), state_map_(longfist::build_state_map(types_)) {}

  template <typename DataType> void operator<<(const state<DataType> &state) {
    auto &target_map = state_map_[boost::hana::type_c<DataType>];
    target_map.insert_or_assign(state.data.uid(), state);
  }

  template <typename DataType> void operator<<(const typed_event_ptr<DataType> &event) {
    auto &target_map = state_map_[boost::hana::type_c<DataType>];
    target_map.insert_or_assign(event->template data<DataType>().uid(), *event);
  }

  void operator>>(const yijinjing::journal::writer_ptr &writer) const {
    boost::hana::for_each(types_, [&](auto it) {
      auto type = boost::hana::second(it);
      for (const auto &element : state_map_[type]) {
        writer->write(0, element.second.data);
      }
    });
  }

  void operator>>(yijinjing::cache::bank &bank) {
    boost::hana::for_each(types_, [&](auto it) {
      auto type = boost::hana::second(it);
      for (const auto &element : state_map_[type]) {
        bank << element.second;
      }
    });
  }

  template <typename DataType>
  const std::unordered_map<uint64_t, state<DataType>> &operator[](const boost::hana::basic_type<DataType> &type) const {
    return state_map_[type];
  }

private:
  DataTypes types_;
  DataTypesMap state_map_;
};

class trading_bank {
public:
  trading_bank()
      : trading_data_map_(longfist::build_ring_state_map(longfist::TradingDataTypes, longfist::TRADING_MAP_RING_SIZE)) {
  }
  template <typename DataType> void operator<<(const state<DataType> &state) {
    auto &target_queue = trading_data_map_[boost::hana::type_c<DataType>];
    target_queue->push(state);
  }

  template <typename DataType> void operator<<(const typed_event_ptr<DataType> &event) {
    auto &target_queue = trading_data_map_[boost::hana::type_c<DataType>];
    kungfu::state<DataType> s(*event);
    target_queue->push(s);
  }

  template <typename DataType>
  kungfu::yijinjing::cache::ringqueue<state<DataType>> &operator[](const boost::hana::basic_type<DataType> &type) {
    return *(trading_data_map_[type]);
  }

  ~trading_bank() {
    boost::hana::for_each(trading_data_map_, [&](const auto &x) { delete (+boost::hana::second(x)); });
  }

private:
  longfist::TradingMapType trading_data_map_;
};
} // namespace kungfu::yijinjing::cache

#endif // KUNGFU_CACHE_RUNTIME_H
