// SPDX-License-Identifier: Apache-2.0

#include <kungfu/yijinjing/journal/tracer.h>

using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::yijinjing::data;

namespace kungfu::yijinjing::journal {

tracer::tracer(const location_ptr location, bool in, bool out, int64_t begin, int64_t end)
    : home_(location), reader_(std::make_shared<reader>(true, false, std::make_shared<bus>(false))), in_(in), out_(out),
      begin_time_(begin), end_time_(end) {

  if (in) {
    auto uid_str = fmt::format("{:08x}", home_->uid);
    auto master_cmd_location = location::make_shared(mode::LIVE, category::SYSTEM, "master", uid_str, get_locator());
    auto master_home_location = location::make_shared(mode::LIVE, category::SYSTEM, "master", "master", get_locator());

    reader_->join(master_cmd_location, home_->uid, begin_time_);
    reader_->join(master_home_location, location::PUBLIC, begin_time_);
  }

  if (out) {
    for (auto dest_id : get_locator()->list_location_dest(home_)) {
      reader_->join(home_, dest_id, begin_time_);
    }
  }

  // init locations_;
  for (auto location : get_locator()->list_locations(".*", ".*", ".*", ".*")) {
    locations_.emplace(location->uid, location);
  }
};

frame_ptr tracer::current_frame() const {
  auto frame = reader_->current_frame();

  if (frame->dest() == home_->uid and frame->msg_type() == RequestReadFrom::tag) {
    auto request = frame->data<RequestReadFrom>();
    auto source_location = locations_.at(request.source_id);
    reader_->join(source_location, home_->uid, begin_time_);
  }
  if (frame->dest() == home_->uid and frame->msg_type() == RequestReadFromPublic::tag) {
    auto request = frame->data<RequestReadFromPublic>();
    auto source_location = locations_.at(request.source_id);
    reader_->join(source_location, location::PUBLIC, begin_time_);
  }
  if (frame->dest() == home_->uid and frame->msg_type() == RequestReadFromSync::tag) {
    auto request = frame->data<RequestReadFromSync>();
    auto source_location = locations_.at(request.source_id);
    reader_->join(source_location, location::SYNC, begin_time_);
  }
  if (frame->dest() == home_->uid and frame->msg_type() == Deregister::tag) {
    reader_->disjoin(location::make_shared(frame->data<Deregister>(), get_locator())->uid);
  }

  return frame;
}
} // namespace kungfu::yijinjing::journal