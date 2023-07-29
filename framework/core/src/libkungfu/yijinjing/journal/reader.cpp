// SPDX-License-Identifier: Apache-2.0

#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/journal/page.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {
reader::~reader() {
  release_page();
  journals_.clear();
}

void reader::join(const data::location_ptr &location, uint32_t dest_id, const int64_t from_time,
                  longfist::enums::Priority priority) {
  SPDLOG_TRACE("join: {}, dest_id: {}", location->to_string(), dest_id);
  auto key = journal_key(location, dest_id);
  auto result = journals_.try_emplace(key, location, dest_id, false, lazy_, low_latency_, bus_, priority);
  if (result.second) {
    journals_.at(key).seek_to_time(from_time);
  }
  if (current_ == nullptr) {
    sort_without_buffer(); // do not sort if current_ is set (because we could be in process of reading)
  }
  buffer_built_ = false;
}

void reader::disjoin(const data::location_ptr &location, uint32_t dest_id) {
  auto key = journal_key(location, dest_id);
  journals_.erase(key);
  current_ = nullptr;
  sort_without_buffer();
}

void reader::disjoin(const uint32_t location_uid) {
  for (auto it = journals_.begin(); it != journals_.end();) {
    if (it->first.location_uid != location_uid) {
      it++;
    } else {
      it = journals_.erase(it);
    }
  }
  current_ = nullptr;
  sort_without_buffer();
}

void reader::disjoin_channel(uint32_t location_uid, uint32_t dest_id) {
  // auto key = static_cast<uint64_t>(location_uid) << 32u | static_cast<uint64_t>(dest_id);
  for (auto it = journals_.begin(); it != journals_.end();) {
    if (it->first.location_uid == location_uid && it->first.dest_id == dest_id) {
      it = journals_.erase(it);
    } else {
      it++;
    }
  }
  current_ = nullptr;
  sort_without_buffer();
}

bool reader::data_available() {
  sort();
  return current_ != nullptr && current_frame()->has_data();
}

void reader::seek_to_time(int64_t nanotime) {
  for (auto &pair : journals_) {
    pair.second.seek_to_time(nanotime);
  }
  sort_without_buffer();
}

void reader::next() {
  if (current_ != nullptr) {
    current_->next();
  }
  sort();
}

void reader::sort_without_buffer() {
  buffer_built_ = false;
  int64_t min_time = time::now_in_nano();
  for (auto &pair : journals_) {
    auto &journal = pair.second;
    auto &frame = journal.current_frame();
    bool current_has_data = current_ != nullptr && current_->current_frame()->has_data();

    if (not current_has_data && frame->has_data() && frame->gen_time() <= min_time) {
      min_time = frame->gen_time();
      current_ = &journal;
      continue;
    }

    if (current_has_data && current_->priority_ < journal.priority_ && frame->has_data()) {
      min_time = frame->gen_time();
      current_ = &journal;
      continue;
    }

    if (current_has_data && current_->priority_ == journal.priority_ && frame->has_data() &&
        frame->gen_time() <= min_time) {
      min_time = frame->gen_time();
      current_ = &journal;
    }
  }

  if (current_ != nullptr and current_->current_frame()->has_data()) {
    SPDLOG_DEBUG("priority: {}, source: {}, dest: {}", current_->priority_, current_->current_frame()->source(),
                 current_->current_frame()->dest());
    SPDLOG_DEBUG("msg_type: {}, data: {}", current_->current_frame()->msg_type(),
                 current_->current_frame()->data_as_string());
  }
}

namespace internal {
template <class ForwardIt, class UnaryPredicate>
ForwardIt swap2tail_if(ForwardIt first, ForwardIt last, const UnaryPredicate &p) {
  first = std::find_if(first, last, p);
  if (first != last) {
    for (ForwardIt i = first; ++i != last;) {
      if (!p(*i))
        std::iter_swap(first++, i);
    }
  }
  return first;
}
} // namespace internal

void reader::sort() {
  if (not buffer_built_) {
    build_buffer();
  }
  int64_t min_time = time::now_in_nano();
  auto has_data_iter = internal::swap2tail_if(no_data_journals_buffer_.begin(), no_data_journals_buffer_.end(),
                                              [](const auto &journal) { return journal->current_frame()->has_data(); });
  for (auto iter = has_data_iter; iter != no_data_journals_buffer_.end(); ++iter) {
    has_data_journals_heap_.push(*iter);
  }
  no_data_journals_buffer_.erase(has_data_iter, no_data_journals_buffer_.end());
  if (has_data_journals_heap_.empty()) {
    return;
  }
  auto min_journal = has_data_journals_heap_.top();
  if (min_journal->current_frame()->gen_time() <= min_time) {
    current_ = min_journal;
    has_data_journals_heap_.pop();
    no_data_journals_buffer_.push_back(current_);
  }
}

void reader::build_buffer() {
  no_data_journals_buffer_.clear();
  has_data_journals_heap_ = {};
  std::transform(journals_.begin(), journals_.end(), std::back_inserter(no_data_journals_buffer_),
                 [](auto &pair) { return std::addressof(pair.second); });
  buffer_built_ = true;
}

bool reader::release_page() {
  bool result = false;
  for (auto &iter : journals_) {
    result |= iter.second.release_page();
  }
  std::lock_guard<std::recursive_mutex> lk(mtx_);
  replica_journals_.clear();
  return result;
}

reader::reader(const reader &other) : lazy_(other.lazy_), low_latency_(other.low_latency_), bus_(other.bus_) {
  for (auto &j : other.journals_) {
    journals_.emplace(j.first, j.second);
    if (other.current_->get_source() == j.second.get_source() and other.current_->get_dest() == j.second.get_dest()) {
      current_ = &(journals_.find(j.first)->second);
    }
  }
}

void reader::keep_only(uint32_t location_uid, uint32_t dest_id) {
  for (auto it = journals_.begin(); it != journals_.end();) {
    if (it->first.location_uid == location_uid && it->first.dest_id == dest_id) {
      it++;
    } else {
      it = journals_.erase(it);
    }
  }
  current_ = nullptr;
  sort_without_buffer();
}

journal &reader::get_journal_ref(const data::location_ptr &location, uint32_t dest_id) {
  auto key = journal_key(location, dest_id);
  auto iter = journals_.find(key);
  if (iter != journals_.end()) {
    return iter->second;
  }

  auto result = journals_.try_emplace(key, location, dest_id, false, lazy_, low_latency_, bus_);
  result.first->second.seek_to_time(time::now_in_nano());
  sort_without_buffer();
  return result.first->second;
}

} // namespace kungfu::yijinjing::journal