// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_TRACER_H
#define KUNGFU_TRACER_H

#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::yijinjing::journal {

typedef std::unordered_map<uint32_t, yijinjing::data::location_ptr> LocationMap;

class tracer {
public:
  explicit tracer(const yijinjing::data::location_ptr location, bool in, bool out, int64_t begin, int64_t end);

  virtual ~tracer() = default;

  void next() { reader_->next(); };

  [[nodiscard]] const yijinjing::data::locator_ptr &get_locator() const { return home_->locator; }

  [[nodiscard]] const yijinjing::data::location_ptr &get_home() const { return home_; }

  [[nodiscard]] bool data_available() const { return reader_->data_available(); };

  [[nodiscard]] yijinjing::journal::frame_ptr current_frame() const;

  [[nodiscard]] yijinjing::journal::page_ptr current_page() const { return reader_->current_page(); };

  // TODO support band
  [[nodiscard]] LocationMap &get_all_locations() { return locations_; }

private:
  yijinjing::data::location_ptr home_;
  yijinjing::journal::reader_ptr reader_;
  LocationMap locations_;
  bool in_;
  bool out_;
  int64_t begin_time_;
  int64_t end_time_;
};

DECLARE_PTR(tracer);

} // namespace kungfu::yijinjing::journal

#endif // KUNGFU_TRACER_H