// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_YIJINJING_FRAME_H
#define KUNGFU_YIJINJING_FRAME_H

#include <kungfu/yijinjing/journal/common.h>

namespace kungfu::yijinjing::journal {
/**
 * Basic memory unit,
 * holds header / data / errorMsg (if needs)
 */
struct frame : event {
  ~frame() override = default;

  [[nodiscard]] bool has_data() const { return header_->length > 0 && header_->msg_type > 0; }

  [[nodiscard]] uintptr_t address() const { return reinterpret_cast<uintptr_t>(header_); }

  [[nodiscard]] uint32_t frame_length() const { return header_->length; }

  [[nodiscard]] uint32_t header_length() const { return header_->header_length; }

  [[nodiscard]] uint32_t data_length() const override { return frame_length() - header_length(); }

  [[nodiscard]] int64_t gen_time() const override { return header_->gen_time; }

  [[nodiscard]] int64_t trigger_time() const override { return header_->trigger_time; }

  [[nodiscard]] int32_t msg_type() const override { return header_->msg_type; }

  [[nodiscard]] uint32_t source() const override { return header_->source; }

  [[nodiscard]] uint32_t initial_source() const override { return header_->initial_source; }

  [[nodiscard]] uint32_t dest() const override { return header_->dest; }

  [[nodiscard]] uint64_t stream_id() const { return header_->stream_id; }

  [[nodiscard]] const void *data_address() const override {
    return reinterpret_cast<void *>(address() + header_length());
  }

  [[nodiscard]] const char *data_as_bytes() const override {
    return reinterpret_cast<char *>(address() + header_length());
  }

  [[nodiscard]] std::vector<uint8_t> data_as_byte_array() const override {
    return {data_as_bytes(), data_as_bytes() + data_length()};
  }

  [[nodiscard]] std::string data_as_string() const override { return std::string{data_as_bytes(), data_length()}; }

  [[nodiscard]] std::string to_string() const override {
    auto j = header_->to_json();
    j["data"] = data_as_string();
    return j.dump(-1, ' ', false, nlohmann::json::basic_json::error_handler_t::replace);
  }

  [[nodiscard]] int8_t data_type() const override { return int8_t(header_->data_type); }

  [[nodiscard]] bool is_json() const override { return data_type() == longfist::enums::FrameDataType::Json; }

  [[nodiscard]] uint64_t frame_uid() const override { return header_->frame_uid; }

  [[nodiscard]] uint64_t trigger_frame_uid() const override { return header_->trigger_frame_uid; }

  template <typename T> size_t copy_data(const T &data) {
    size_t length = sizeof(T);
    memcpy(const_cast<void *>(data_address()), &data, length);
    return length;
  }

  void set_stream_id(uint64_t stream_id) { header_->stream_id = stream_id; }

private:
  longfist::types::frame_header *header_ = nullptr;

  frame() = default;

  void set_address(uintptr_t address) { header_ = reinterpret_cast<longfist::types::frame_header *>(address); }

  void move_to_next() { set_address(address() + frame_length()); }

  void set_header_length() { header_->header_length = sizeof(longfist::types::frame_header); }

  void set_data_length(uint32_t length) { header_->length = header_length() + length; }

  void set_gen_time(int64_t gen_time) { header_->gen_time = gen_time; }

  void set_trigger_time(int64_t trigger_time) { header_->trigger_time = trigger_time; }

  void set_msg_type(int32_t msg_type) { header_->msg_type = msg_type; }

  void set_data_type(longfist::enums::FrameDataType data_type) { header_->data_type = data_type; }

  void set_source(uint32_t source) { header_->source = source; }

  void set_initial_source(uint32_t initial_source) { header_->initial_source = initial_source; }

  void set_dest(uint32_t dest) { header_->dest = dest; }

  void set_frame_uid(uint64_t frame_uid) { header_->frame_uid = frame_uid; }

  void set_trigger_frame_uid(uint64_t trigger_frame_uid) { header_->trigger_frame_uid = trigger_frame_uid; }

  void copy(const frame &source) { memcpy(header_, source.header_, source.frame_length()); }

  friend struct cloned_frame;

  friend class journal;

  friend class writer;

  friend class replay_writer;
};

struct cloned_frame : frame {
  cloned_frame() : frame() {}

  ~cloned_frame() override { free(header_); };

  void copy(frame &from) {
    header_ = reinterpret_cast<longfist::types::frame_header *>(malloc(from.frame_length()));
    memset(header_, 0, from.frame_length());
    memcpy(header_, from.header_, from.frame_length());
  }

  void open(uint32_t data_length) {
    auto frame_length = sizeof(longfist::types::frame_header) + data_length;
    header_ = reinterpret_cast<longfist::types::frame_header *>(malloc(frame_length));
    memset(header_, 0, frame_length);
  }
};

} // namespace kungfu::yijinjing::journal
#endif // KUNGFU_YIJINJING_FRAME_H
