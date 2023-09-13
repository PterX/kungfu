// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::yijinjing::journal {

replay_writer::replay_writer(const data::location_ptr &location, uint32_t dest_id, publisher_ptr publisher,
                             const bus_ptr &bus, uint32_t page_size, int64_t begin_time)
    : writer(location, dest_id, true, publisher, false, bus, page_size, begin_time),
      reader_for_write_(std::make_shared<reader>(true, false, bus)) {
  if (page::check_page_existed(location, dest_id)) {
    reader_for_write_->join(location, dest_id, begin_time, page_size);
  } else {
    SPDLOG_ERROR("page not existed, location: {}, dest: {}", location->uname, dest_id);
    throw yijinjing_error("page not existed");
  }
}

frame_ptr replay_writer::open_frame(int64_t trigger_time, int32_t msg_type, uint32_t length) {
  while (reader_for_write_->data_available()) {
    auto frame = reader_for_write_->current_frame();
    if (frame->msg_type() == msg_type and frame->gen_time() >= trigger_time) {
      break;
    }
    reader_for_write_->next();
    journal_.next();
  }

  if (not reader_for_write_->data_available()) {
    SPDLOG_ERROR("no more data available");
    throw yijinjing_error("no more data available");
  }

  cloned_frame_.copy(*reader_for_write_->current_frame());
  return std::make_shared<cloned_frame>(cloned_frame_);
}

void replay_writer::close_frame(size_t data_length, int64_t gen_time) {
  cloned_frame_.copy(*reader_for_write_->current_frame());
  reader_for_write_->next();
  journal_.next();
}
} // namespace kungfu::yijinjing::journal