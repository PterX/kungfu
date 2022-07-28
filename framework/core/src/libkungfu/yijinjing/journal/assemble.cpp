
//
// Created by Keren Dong on 2020/5/22.
//

#include <kungfu/yijinjing/cache/backend.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/assemble.h>
#include <kungfu/yijinjing/time.h>
namespace kungfu::yijinjing::journal {
struct noop_publisher : public publisher {
  noop_publisher() = default;
  bool is_usable() override { return true; }
  void setup() override {}
  int notify() override { return 0; }
  int publish(const std::string &json_message) override { return 0; }
};

struct assemble_exception : std::runtime_error {
  explicit assemble_exception(const std::string &msg) : std::runtime_error(msg){};
};

sink::sink() : publisher_(std::make_shared<noop_publisher>()) {}

publisher_ptr sink::get_publisher() { return publisher_; }

copy_sink::copy_sink(data::locator_ptr locator) : sink(), locator_(std::move(locator)) {}

void copy_sink::put(const data::location_ptr &location, uint32_t dest_id, const frame_ptr &frame) {
  auto pair = writers_.try_emplace(location->uid);
  auto &writers = pair.first->second;
  if (writers.find(dest_id) == writers.end()) {
    auto target_location = data::location::make_shared(*location, locator_);
    writers.try_emplace(dest_id, std::make_shared<writer>(target_location, dest_id, true, get_publisher()));
  }
  writers.at(dest_id)->copy_frame(frame);
}

assemble::assemble(const std::string &mode, const std::string &category,
                   const std::string &group, const std::string &name)
                    : mode_(mode), category_(category), group_(group), name_(name)  { }



using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace sqlite_orm;

std::vector<kungfu::longfist::types::Session> assemble::get_sessions() {
  kungfu::yijinjing::data::locator_ptr l(new kungfu::yijinjing::data::locator());
  auto index_location =
      kungfu::yijinjing::data::location::make_shared(mode::LIVE, category::SYSTEM, "journal", "index", l);
  std::string session_db = l->layout_file(index_location, layout::SQLITE, "index");
  kungfu::yijinjing::cache::SessionStoragePtr session_storage_(
      cache::make_storage_ptr(session_db, kungfu::longfist::SessionDataTypes));
  if (not session_storage_->sync_schema_simulate().empty()) {
    session_storage_->sync_schema();
  }
  auto bt = &Session::begin_time;
  auto range = where(greater_or_equal(bt, 0) and lesser_or_equal(bt, INT64_MAX));
  return session_storage_->get_all<Session>(range, order_by(bt));
}
} // namespace kungfu::yijinjing::journal