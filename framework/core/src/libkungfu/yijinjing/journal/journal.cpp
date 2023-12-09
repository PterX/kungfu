// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {

journal::~journal() {
  if (page_) {
    page_.reset();
  }
  release_page();
}

void journal::next() {
  assert(page_.get() != nullptr);
  if (frame_->msg_type() == longfist::types::PageEnd::tag) {
    load_next_page();
    try_load_next_extra_page();
  } else {
    frame_->move_to_next();
    page_frame_nb_++;
  }
}

void journal::seek_to_time(int64_t nanotime) {
  int page_id = page::find_page_id(location_, dest_id_, nanotime);
  load_page(page_id);
  while (page_->is_full() && page_->end_time() <= nanotime) {
    load_next_page();
  }
  try_load_next_extra_page();
  while (frame_->has_data() && frame_->gen_time() <= nanotime) {
    next();
  }
}

void journal::load_page(int page_id) {
  if (page_.get() == nullptr or page_->get_page_id() != page_id) {

    if (page_.get() != nullptr && bus_->is_on_load_page_required()) {
      std::lock_guard<std::recursive_mutex> lk(passed_page_collector_mtx_);
      passed_page_collector_.push_back(std::move(page_));
      bus_->on_load_page();
    }

    page_ = page::load(location_, dest_id_, page_size_, page_id, is_writing_, lazy_);
  }

  frame_->set_address(page_->first_frame_address());
  page_frame_nb_ = 0u;
}

void journal::load_next_page() { load_page(page_->get_page_id() + 1); }

// saving time for other process switch page, except the master
// only for master reading, and low_latency mode
void journal::try_load_next_extra_page() {
  if (lazy_ || is_writing_ || !low_latency_ || page_size_ != page::find_page_size(location_, dest_id_)) {
    return;
  }
  pre_page_ = page::load(location_, dest_id_, page_->get_page_size(), page_->get_page_id() + 1, false, lazy_, true);
}

bool journal::release_page() {
  static thread_local std::vector<page_ptr> queue_release_page{};
  {
    std::lock_guard<std::recursive_mutex> lk(passed_page_collector_mtx_);
    if (passed_page_collector_.empty()) {
      return false;
    }

    for (auto &page : passed_page_collector_) {
      queue_release_page.push_back(std::move(page));
    }
    passed_page_collector_.clear();
  }

  for (auto &page : queue_release_page) {
    // wait for the main thread to release shared_ptr<page>, or page would close in the main thread
    while (page.use_count() > 1 and not replica_) {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    page.reset();
  }
  queue_release_page.clear();

  return true;
}

journal::journal(const journal &other)
    : location_(other.location_), dest_id_(other.dest_id_), page_size_(other.page_size_),
      is_writing_(other.is_writing_), lazy_(other.lazy_), low_latency_(other.low_latency_), bus_(other.bus_),
      page_frame_nb_(other.page_frame_nb_), priority_(other.priority_) {
  pre_page_ = other.pre_page_;
  page_ = other.page_;
  frame_ = std::make_shared<frame>(*other.frame_);
  replica_ = true;
}

} // namespace kungfu::yijinjing::journal