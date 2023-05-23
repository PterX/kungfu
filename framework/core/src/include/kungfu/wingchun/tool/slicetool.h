
#ifndef KUNGFU_TOOL_SLICE_TOOL_H
#define KUNGFU_TOOL_SLICE_TOOL_H

#include <kungfu/common.h>
#include <tuple>
#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/tool/sliceindexer.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::wingchun::tool {

class SliceTool {

public:
  SliceTool(longfist::enums::category category, std::string group, std::string name, SliceIndexer_ptr indexer,
            bool overwrite = true);

  virtual ~SliceTool() = default;

  int64_t get_begin_time() const { return indexer_->get_begin_time(); }

  int64_t get_end_time() const { return indexer_->get_end_time(); }

  //   yijinjing::data::location_ptr get_location() const { return cache_location_; }

  virtual void run(){};

  // template <typename DataType, std::enable_if_t<std::is_same_v<DataType, longfist::types::Quote> or
  //                                               std::is_same_v<DataType, longfist::types::Entrust> or
  //                                               std::is_same_v<DataType, longfist::types::Transaction> or
  //                                               std::is_same_v<DataType, longfist::types::Tree>>...>
  template <typename DataType>
  void write_at(int64_t gen_time, int64_t trigger_time, uint32_t dest_id, const DataType &data) {
    valid_time(gen_time, trigger_time);
    auto md_location = find_md_slice_location(gen_time, data.instrument_id, data.exchange_id, DataType::tag);
    auto writer = get_writer(md_location, dest_id);
    writer->write_at(gen_time, trigger_time, data);
  }

  // template <typename DataType, std::enable_if_t<std::is_same_v<DataType, longfist::types::SyntheticData>>...>
  // void write_at(int64_t gen_time, int64_t trigger_time, uint32_t dest_id, const DataType &data) {
  //   valid_time(gen_time, trigger_time);
  //   auto op_location = find_operator_slice_location(gen_time);
  //   auto writer = get_writer(op_location, dest_id);
  //   writer->write_at(gen_time, trigger_time, data);
  // }

  yijinjing::data::location_ptr find_md_slice_location(int64_t nano_time, const std::string &instrument_id,
                                                       const std::string &exchange_id, int32_t data_type) const;

  yijinjing::data::location_ptr find_operator_slice_location(int64_t nano_time) const;

protected:
  // void write_raw_at(int64_t gen_time, int64_t trigger_time, uint32_t dest_id, int32_t msg_type, uintptr_t data,
  //                   uint32_t length);

  // void write_raw_at_as(int64_t gen_time, int64_t trigger_time, uint32_t source, uint32_t dest_id, int32_t msg_type,
  //                      uintptr_t data, uint32_t length);

  longfist::enums::category category_;
  std::string group_;
  std::string name_;
  SliceIndexer_ptr indexer_;
  bool overwrite_;
  yijinjing::publisher_ptr publisher_;
  mutable std::map<std::tuple<std::string, uint32_t>, yijinjing::practice::WriterMap> writer_maps_;
  yijinjing::journal::reader_ptr reader_;
  mutable int64_t last_gen_time_;
  mutable int64_t last_read_gen_time_;

  void next();

  bool data_available() const;

  int64_t get_last_read_gen_time() const { return last_read_gen_time_; }

  void join(const yijinjing::data::location_ptr &location, uint32_t dest_id, const int64_t from_time);

  yijinjing::journal::frame_ptr current_frame() const;

  yijinjing::journal::writer_ptr get_writer(const yijinjing::data::location_ptr &location, uint32_t dest_id);

  // void init(bool overwrite);

  void valid_time(int64_t gen_time, int64_t trigger_time) const ;
};

} // namespace kungfu::wingchun::tool

#endif // KUNGFU_TOOL_SLICE_TOOL_H