
// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/5/22.
//

#include <kungfu/yijinjing/cache/backend.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/journal/assemble.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {
using namespace longfist::enums;
using namespace longfist::types;

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
    writers.try_emplace(dest_id, std::make_shared<writer>(target_location, dest_id, true, get_publisher(), false,
                                                          std::make_shared<bus>(false)));
  }
  writers.at(dest_id)->copy_frame(frame);
}

assemble::assemble(const std::string &mode, const std::string &category, const std::string &group,
                   const std::string &name)
    : mode_(mode), category_(category), group_(group), name_(name), publisher_(std::make_shared<noop_publisher>()) {}

assemble::assemble(const std::vector<data::locator_ptr> &locators, const std::string &mode, const std::string &category,
                   const std::string &group, const std::string &name)
    : mode_(mode), category_(category), group_(group), name_(name), publisher_(std::make_shared<noop_publisher>()) {
  for (auto &locator : locators) {
    locators_.push_back(locator);
    readers_.push_back(std::make_shared<reader>(true, false, std::make_shared<bus>(false)));
    auto reader = readers_.back();
    for (auto &location : locator->list_locations(category, group, name, mode)) {
      for (auto dest_id : locator->list_location_dest(location)) {
        reader->join(location, dest_id, 0);
      }
    }
  }
  sort();
}

assemble assemble::operator+(assemble &other) {
  if (mode_ != other.mode_ or category_ != other.category_ or group_ != other.group_ or name_ != other.name_) {
    auto msg = fmt::format("assemble incompatible: {}/{}/{}/{}, {}/{}/{}/{}", category_, group_, name_, mode_,
                           other.category_, other.group_, other.name_, other.mode_);
    throw assemble_exception(msg);
  }
  std::vector<data::locator_ptr> merged_locators = {};
  merged_locators.insert(merged_locators.end(), locators_.begin(), locators_.end());
  merged_locators.insert(merged_locators.end(), other.locators_.begin(), other.locators_.end());
  return assemble(merged_locators, mode_, category_, group_, name_);
}

assemble &assemble::operator+=(const assemble &other) {
  // add journals in the same locator
  std::unordered_set<uint32_t> other_same_index;
  for (uint32_t other_locator_index = 0; other_locator_index < other.locators_.size(); ++other_locator_index) {
    const auto &other_locator = other.locators_.at(other_locator_index);
    for (uint32_t this_locator_index = 0; this_locator_index < locators_.size(); ++this_locator_index) {
      const auto &this_locator = locators_.at(this_locator_index);
      if (this_locator == other_locator) {
        auto &this_reader = readers_.at(this_locator_index);
        const auto &other_reader = other.readers_.at(other_locator_index);
        for (const auto &other_pair : other_reader->get_journals()) {
          const auto &other_journal = other_pair.second;
          this_reader->join(other_journal.get_location(), other_journal.get_dest(), other.from_time_);
        }
        other_same_index.emplace(other_locator_index);
        break;
      }
    }
  }

  // add journals of other, whose locator not in locators_ of this
  for (uint32_t other_locator_index = 0; other_locator_index < locators_.size(); ++other_locator_index) {
    if (other_same_index.find(other_locator_index) != other_same_index.end()) {
      continue;
    }
    const auto &other_locator = other.locators_.at(other_locator_index);
    const auto &other_reader = other.readers_.at(other_locator_index);
    locators_.push_back(other_locator);
    readers_.push_back(std::make_shared<reader>(true, false, std::make_shared<bus>(false)));
    auto &this_reader = readers_.back();
    for (const auto &other_pair : other_reader->get_journals()) {
      const auto &other_journal = other_pair.second;
      this_reader->join(other_journal.get_location(), other_journal.get_dest(), other.from_time_);
    }
  }

  return *this;
}

assemble &assemble::operator-=(const assemble &other) {
  for (uint32_t other_locator_index = 0; other_locator_index < other.locators_.size(); ++other_locator_index) {
    const auto &other_locator = other.locators_.at(other_locator_index);
    for (uint32_t this_locator_index = 0; this_locator_index < locators_.size(); ++this_locator_index) {
      const auto &this_locator = locators_.at(this_locator_index);
      if (this_locator == other_locator) {
        auto &this_reader = readers_.at(this_locator_index);
        const auto &other_reader = other.readers_.at(other_locator_index);
        for (const auto &other_pair : other_reader->get_journals()) {
          const auto &other_journal = other_pair.second;
          this_reader->disjoin_channel(other_journal.get_location()->location_uid, other_journal.get_dest());
        }
        break;
      }
    }
  }
  return *this;
}

void assemble::operator>>(const sink_ptr &sink) {
  while (data_available()) {
    auto page = current_reader_->current_page();
    sink->put(page->get_location(), page->get_dest_id(), current_frame());
    next();
  }
}

bool assemble::data_available() {
  sort();
  return std::any_of(readers_.begin(), readers_.end(), [](auto &r) { return r->data_available(); });
}

void assemble::next() {
  if (current_reader_ and current_reader_->data_available()) {
    current_reader_->next();
  }
  sort();
}

frame_ptr assemble::current_frame() { return current_reader_->current_frame(); }

void assemble::sort() {
  int64_t min_time = INT64_MAX;
  for (auto &reader : readers_) {
    if (reader->data_available() and reader->current_frame()->gen_time() < min_time) {
      min_time = reader->current_frame()->gen_time();
      current_reader_ = reader;
    }
  }
}
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace sqlite_orm;

std::vector<kungfu::longfist::types::Session> assemble::get_sessions(const kungfu::yijinjing::data::location_ptr &pl) {
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
  auto sessions = session_storage_->get_all<Session>(range, order_by(bt));
  if (!pl) {
    return sessions;
  } else {
    std::vector<kungfu::longfist::types::Session> filtered_sessions;
    std::copy_if(sessions.begin(), sessions.end(), std::back_inserter(filtered_sessions),
                 [&pl](kungfu::longfist::types::Session x) { return x.location_uid == pl->location_uid; });
    return filtered_sessions;
  }
}

[[maybe_unused]] std::shared_ptr<frame_reader> assemble::get_reader(const kungfu::yijinjing::data::location_ptr &pl) {
  auto io_dvc = std::make_shared<kungfu::yijinjing::io_device>(pl, true, true);
  auto curr = std::chrono::system_clock::now();
  time_t tm = std::chrono::system_clock::to_time_t(curr);
  auto tm_begin = std::localtime(&tm);
  tm_begin->tm_hour = 0;
  tm_begin->tm_min = 0;
  tm_begin->tm_sec = 0;
  std::time_t t_begin = std::mktime(tm_begin);
  int64_t begin_time = t_begin * 1000000000;
  auto tm_end = std::localtime(&tm);
  tm_end->tm_hour = 23;
  tm_end->tm_min = 59;
  tm_end->tm_sec = 59;
  std::time_t t_end = std::mktime(tm_end);
  int64_t end_time = t_end * 1000000000 + 999999999;
  auto p_reader = std::make_shared<frame_reader>(io_dvc, begin_time, end_time, true);
  auto uid_str = fmt::format("{:08x}", io_dvc->get_home()->uid);
  auto master_cmd_location = kungfu::yijinjing::data::location::make_shared(mode::LIVE, category::SYSTEM, "master",
                                                                            uid_str, io_dvc->get_locator());
  auto master_home_location = kungfu::yijinjing::data::location::make_shared(mode::LIVE, category::SYSTEM, "master",
                                                                             "master", io_dvc->get_locator());

  p_reader->join(master_cmd_location, io_dvc->get_home()->uid, begin_time);
  p_reader->join(master_home_location, kungfu::yijinjing::data::location::PUBLIC, begin_time);
  for (auto dest_id : io_dvc->get_locator()->list_location_dest(io_dvc->get_home())) {
    p_reader->join(io_dvc->get_home(), dest_id, begin_time);
  }
  return p_reader;
}

assemble::assemble(const data::location_ptr &source_location, uint32_t dest_id, uint32_t assemble_mode,
                   int64_t from_time)
    : assemble() {
  from_time_ = from_time;
  locators_.clear();
  readers_.clear();
  data::locator &l = *source_location->locator;
  locators_.push_back(source_location->locator);
  readers_.push_back(std::make_shared<reader>(true, false, std::make_shared<bus>(false)));
  auto reader = readers_.front();

  // join channel
  if (assemble_mode & AssembleMode::Channel) {
    reader->join(source_location, dest_id, from_time);
  }

  // join all journal dest of location
  if (assemble_mode & AssembleMode::Write) {
    for (auto dest : l.list_location_dest(source_location)) {
      reader->join(source_location, dest, from_time);
    }
  }

  // scan all locations, join dest_id or PUBLIC
  bool b_read = assemble_mode & AssembleMode::Read;
  bool b_public = assemble_mode & AssembleMode::Public;
  bool b_sync = assemble_mode & AssembleMode::Sync;
  bool b_all = assemble_mode & AssembleMode::All;
  if (b_read or b_public or b_all) {
    for (auto &location : l.list_locations("*", "*", "*", "*")) {
      for (auto dest : l.list_location_dest(location)) {
        if (b_all) {
          reader->join(location, dest, from_time);
        } else if (b_read and dest == dest_id) {
          reader->join(location, dest_id, from_time);
        } else if (b_public and dest == data::location::PUBLIC) {
          reader->join(location, data::location::PUBLIC, from_time);
        } else if (b_sync and dest == data::location::SYNC) {
          reader->join(location, data::location::SYNC, from_time);
        }
      }
    }
  }
  sort();
}

[[maybe_unused]] void assemble::seek_to_time(int64_t nano_time) {
  for (auto &reader : readers_) {
    reader->seek_to_time(nano_time);
  }
  sort();
}

[[maybe_unused]] std::vector<frame_header> assemble::read_headers(int32_t msg_type, int64_t end_time) {
  std::vector<frame_header> v{};
  while (data_available() and current_frame()->gen_time() < end_time) {
    if (msg_type == 0 or current_frame()->msg_type() == msg_type) {
      v.push_back(*reinterpret_cast<frame_header *>(current_frame()->address()));
    }
    next();
  }
  return v;
}

std::vector<std::pair<longfist::types::frame_header, std::vector<uint8_t>>> assemble::read_bytes(int32_t msg_type,
                                                                                                 int64_t end_time) {
  std::vector<std::pair<longfist::types::frame_header, std::vector<uint8_t>>> v{};
  while (data_available() and current_frame()->gen_time() < end_time) {
    if (msg_type == 0 or current_frame()->msg_type() == msg_type) {
      const frame_header &head = *reinterpret_cast<frame_header *>(current_frame()->address());
      std::vector<uint8_t> bytes{current_frame()->data_as_bytes(),
                                 current_frame()->data_as_bytes() + current_frame()->data_length()};
      v.emplace_back(head, bytes);
    }
    next();
  }
  return v;
}

void assemble::disjoin(uint32_t location_uid) {
  for (auto &reader : readers_) {
    reader->disjoin(location_uid);
  }
}

void assemble::disjoin_channel(uint32_t location_uid, uint32_t dest_id) {
  for (auto &reader : readers_) {
    reader->disjoin_channel(location_uid, dest_id);
  }
}

} // namespace kungfu::yijinjing::journal