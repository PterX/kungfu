// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {

journal::journal(data::location_ptr location, uint32_t dest_id, bool is_writing, bool lazy, bool low_latency,
                 bus_ptr bus, uint64_t page_size, longfist::enums::Priority priority)
    : location_(std::move(location)), dest_id_(dest_id), is_writing_(is_writing), lazy_(lazy),
      low_latency_(low_latency), bus_(std::move(bus)), frame_(std::shared_ptr<frame>(new frame())), page_frame_nb_(0u),
      page_size_(page_size), priority_(priority), replica_(false) {
  char *keep_page = std::getenv("KF_KEEP_PAGE");
  char *preload = std::getenv("KF_PRELOAD");
  keep_page_ = keep_page != nullptr;
  preload_ = preload != nullptr;
  SPDLOG_DEBUG("keep_page_: {}, preload_: {}", keep_page_, preload_);
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

journal::~journal() {
  if (page_) {
    page_.reset();
  }
  keep_page_ = false;
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
  uint32_t page_id = page::find_page_id(location_, dest_id_, nanotime);
  load_page(page_id);
  while (page_->is_full() && page_->end_time() <= nanotime) {
    load_next_page();
  }
  try_load_next_extra_page();
  while (frame_->has_data() && frame_->gen_time() <= nanotime) {
    next();
  }
}

void journal::load_page(uint32_t page_id) {
  {
    std::lock_guard<std::recursive_mutex> lk_load_page(load_page_mtx_);
    if (not page_ or page_->get_page_id() != page_id) {
      if (page_ and (keep_page_ or bus_->is_on_load_page_required())) {
        std::lock_guard<std::recursive_mutex> lk_passed_page(passed_page_collector_mtx_);
        passed_page_collector_.push_back(std::move(page_));
      }

      if (preload_ and preload_page_ and preload_page_->get_page_id() == page_id) {
        SPDLOG_DEBUG("assign preload_page_ {} to page_ , ", preload_page_->get_page_id());
        page_ = std::move(preload_page_);
      } else {
        page_ = page::load(location_, dest_id_, page_size_, page_id, is_writing_, lazy_);
      }
    }
    frame_->set_address(page_->first_frame_address());
    page_frame_nb_ = 0u;
  }
  if (preload_ or bus_->is_on_load_page_required()) {
    bus_->on_load_page();
  }
}

void journal::load_next_page() { load_page(page_->get_page_id() + 1); }

void journal::preload_next_page() {
  std::lock_guard<std::recursive_mutex> lk(load_page_mtx_);
  if ((not preload_ or not page_) or                                                       //
      (preload_page_ and preload_page_->get_page_id() == page_->get_page_id() + 1) or      //
      (page_->header_->status == longfist::enums::PageStatus::PreOpen) or                  //
      (not page::check_page_existed(location_, page_->dest_id_, page_->get_page_id() + 1)) //
  ) {
    SPDLOG_TRACE("page_->header_->status: {}", page_->header_->status);
    return;
  }

  SPDLOG_TRACE("preload_next_page: {}, {}->{}", page_->get_page_id() + 1, location_->uname, dest_id_);
  preload_page_ = page::load(location_, dest_id_, page_size_, page_->get_page_id() + 1, is_writing_, lazy_);
}

// saving time for other process switch page, except the master
// only for master reading, and low_latency mode
void journal::try_load_next_extra_page() {
  if (lazy_ || is_writing_ || !low_latency_ || page_size_ != page::find_page_size(location_, dest_id_)) {
    return;
  }
  pre_page_ = page::load(location_, dest_id_, page_->get_page_size(), page_->get_page_id() + 1, false, lazy_, true);
}

void journal::release_page() {
  if (keep_page_) {
    SPDLOG_TRACE("keep_page_: {}, {}->{}", keep_page_, location_->uname, dest_id_);
    return;
  }

  static thread_local std::vector<page_ptr> queue_release_page{};
  {
    std::lock_guard<std::recursive_mutex> lk_passed_page(passed_page_collector_mtx_);
    if (passed_page_collector_.empty()) {
      return;
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
}

void journal::close_page(int64_t trigger_time, int64_t last_gen_time) {
  page_ptr last_page = page_;

  // must load_next_page before mark PageEnd::tag,
  // or reader of other process may read next page before it created
  load_next_page();

  frame last_page_frame;
  last_page_frame.set_address(last_page->last_frame_address());
  last_page_frame.move_to_next();
  last_page_frame.set_header_length();
  last_page_frame.set_trigger_time(trigger_time);
  last_page_frame.set_msg_type(longfist::types::PageEnd::tag);
  last_page_frame.set_source(location_->uid);
  last_page_frame.set_dest(dest_id_);
  last_page_frame.set_gen_time(last_gen_time);
  last_page_frame.set_data_length(0);
  last_page->set_last_frame_position(last_page_frame.address() - last_page->address());
}

} // namespace kungfu::yijinjing::journal