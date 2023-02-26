// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/yijinjing/journal/page.h>
#include <kungfu/yijinjing/util/os.h>

namespace kungfu::yijinjing::journal {
page::page(data::location_ptr location, uint32_t dest_id, const uint32_t page_id, const size_t size, const bool lazy,
           const bool is_writing, uintptr_t address)
    : location_(std::move(location)), dest_id_(dest_id), page_id_(page_id), lazy_(lazy), size_(size),
      is_writing_(is_writing), header_(reinterpret_cast<page_header *>(address)) {
  assert(address > 0);
}

page::~page() {
  auto start = std::chrono::system_clock::now();

  if (not os::release_mmap_buffer(address(), size_, lazy_)) {
    SPDLOG_ERROR("can not release page {}/{:08x}.{}.journal", location_->uname, dest_id_, page_id_);
  }

  auto end = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  SPDLOG_INFO("release_mmap_buffer   {}/{:08x}.{}.journal take time {} lazy {} size {} is_writing {}", location_->uname,
              dest_id_, page_id_, duration.count(), lazy_, size_, is_writing_);
}

void page::flush() {
  auto start = std::chrono::system_clock::now();

  if (not os::flush_mmap_buffer(address(), size_, lazy_)) {
    SPDLOG_ERROR("can not flush page {}/{:08x}.{}.journal", location_->uname, dest_id_, page_id_);
  }

  auto end = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  SPDLOG_DEBUG("flush_mmap_buffer   {}/{:08x}.{}.journal take time {} lazy {} size {} is_writing {}", location_->uname,
               dest_id_, page_id_, duration.count(), lazy_, size_, is_writing_);
}

void page::set_last_frame_position(uint64_t position) {
  const_cast<page_header *>(header_)->last_frame_position = position;
}

page_ptr page::load(const data::location_ptr &location, uint32_t dest_id, uint32_t page_id, bool is_writing, bool lazy,
                    bool pre_open) {
  uint32_t page_size = find_page_size(location, dest_id);
  std::string path = get_page_path(location, dest_id, page_id);
  uintptr_t address = os::load_mmap_buffer(path, page_size, is_writing, lazy);

  if (address < 0) {
    throw journal_error("unable to load page for " + path);
  }

  auto header = reinterpret_cast<page_header *>(address);
  if (header->last_frame_position == 0) {
    header->version = __JOURNAL_VERSION__;
    header->page_header_length = sizeof(page_header);
    header->page_size = page_size;
    header->frame_header_length = sizeof(frame_header);
    header->last_frame_position = header->page_header_length;
  }

  if (pre_open) {
    header->status = longfist::enums::PageStatus::PreOpen;
  } else if (is_writing) {
    header->status = longfist::enums::PageStatus::Normal;
  }

  if (header->version != __JOURNAL_VERSION__) {
    uint32_t v = header->version;
    throw journal_error(fmt::format("{} version mismatch, required {}, found {}", path, __JOURNAL_VERSION__, v));
  }
  if (header->page_header_length != sizeof(page_header)) {
    uint32_t l = header->page_header_length;
    throw journal_error(fmt::format("{} header length mismatch, required {}, found {}", path, sizeof(page_header), l));
  }
  if (header->page_size != page_size) {
    uint32_t s = header->page_size;
    throw journal_error(
        fmt::format("page size mismatch, required {}, found {}, location {}, path {}, dest_id {}, page_id {}",
                    page_size, s, location->uname, path, dest_id, page_id));
  }

  if (header->status != longfist::enums::PageStatus::Normal && !pre_open) {
    SPDLOG_WARN("page is still preopen status");
  }

  return std::shared_ptr<page>(new page(location, dest_id, page_id, page_size, lazy, is_writing, address));
}

page_ptr page::load_header_and_1st_frame_header(const data::location_ptr &location, uint32_t dest_id, uint32_t page_id,
                                                bool is_writing, bool lazy) {
  uint32_t page_size = find_page_size(location, dest_id);
  uint32_t page_header_size = sizeof(page_header);
  uint32_t frame_header_size = sizeof(frame_header);
  uint32_t sliced_page_size = page_header_size + frame_header_size;
  std::string path = get_page_path(location, dest_id, page_id);
  uintptr_t address = os::load_mmap_buffer(path, sliced_page_size, is_writing, lazy);

  if (address < 0) {
    throw journal_error("unable to load page for " + path);
  }

  auto header = reinterpret_cast<page_header *>(address);
  if (header->last_frame_position == 0) {
    throw journal_error("Ops, open a page never loaded " + path);
  }

  if (header->version != __JOURNAL_VERSION__) {
    uint32_t v = header->version;
    throw journal_error(fmt::format("{} version mismatch, required {}, found {}", path, __JOURNAL_VERSION__, v));
  }
  if (header->page_header_length != sizeof(page_header)) {
    uint32_t l = header->page_header_length;
    throw journal_error(fmt::format("{} header length mismatch, required {}, found {}", path, sizeof(page_header), l));
  }
  if (header->page_size != page_size) {
    uint32_t s = header->page_size;
    throw journal_error(
        fmt::format("page size mismatch, required {}, found {}, location {}, path {}, dest_id {}, page_id {}",
                    page_size, s, location->uname, path, dest_id, page_id));
  }

  return std::shared_ptr<page>(new page(location, dest_id, page_id, sliced_page_size, lazy, is_writing, address));
}

std::string page::get_page_path(const data::location_ptr &location, uint32_t dest_id, uint32_t page_id) {
  auto page_name = fmt::format("{:08x}.{}", dest_id, page_id);
  return location->locator->layout_file(location, longfist::enums::layout::JOURNAL, page_name);
}

uint32_t page::find_page_id(const data::location_ptr &location, uint32_t dest_id, int64_t time) {
  std::vector<uint32_t> page_ids = location->locator->list_page_id(location, dest_id);
  if (page_ids.empty()) {
    return 1;
  }
  if (time == 0) {
    return page_ids.front();
  }
  for (int i = static_cast<int>(page_ids.size()) - 1; i >= 0; i--) {
    auto loaded_page = page::load_header_and_1st_frame_header(location, dest_id, page_ids[i], false, true);
    auto &loaded_page_header = loaded_page->header_;
    if (loaded_page_header->status != longfist::enums::PageStatus::PreOpen && loaded_page->begin_time() < time) {
      return page_ids[i];
    }
  }
  return page_ids.front();
}
} // namespace kungfu::yijinjing::journal
