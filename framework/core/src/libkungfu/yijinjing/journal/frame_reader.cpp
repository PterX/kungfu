#include <kungfu/yijinjing/journal/frame_reader.h>
namespace kungfu::yijinjing::journal {
using namespace kungfu::longfist::types;
frame_ptr frame_reader::next_frame() {
    // SPDLOG_INFO("Next data_available() {} current_frame()->gen_time() {} end_time_ {}",data_available(), current_frame()->gen_time(), end_time_);
  if (data_available() && current_frame()->gen_time() <= end_time_) {
    //   if (current_frame()->gen_time() >= begin_time_) {
    std::unordered_map<uint32_t, kungfu::yijinjing::data::location_ptr> locations = {};
    for (auto location : io_device_->get_home()->locator->list_locations(".*", ".*", ".*", ".*")) {
      locations.emplace(location->uid, location);
    }
    auto frame = current_frame();
    auto dest_name = frame->dest() == yijinjing::data::location::PUBLIC ? "public" : locations.at(frame->dest())->uname;
    bool type_found = false;
    boost::hana::for_each(kungfu::longfist::AllTypes, [&](auto type) {
      using DataType = typename decltype(+boost::hana::second(type))::type;
      if (frame->msg_type() == DataType::tag) {
        SPDLOG_INFO("Next {} {} {} {} {} {}", time::strftime(frame->gen_time(), "%T.%N"),
                    time::strftime(frame->trigger_time(), "%T.%N"), locations.at(frame->source())->uname, dest_name,
                    DataType::type_name.c_str(), frame->data<DataType>().to_string());
        type_found = true;
      }
    });
    if (not type_found) {
      auto location_uname = current_page()->get_location()->uname;
      auto dest_id = current_page()->get_dest_id();
      SPDLOG_ERROR("{}/{:08x} msg_type {} not found", location_uname, dest_id, frame->msg_type());
      return {};
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFrom::tag) {
      auto request = frame->data<RequestReadFrom>();
      auto source_location = locations.at(request.source_id);
      join(source_location, io_device_->get_home()->uid, request.from_time);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromPublic::tag) {
      auto request = frame->data<RequestReadFromPublic>();
      auto source_location = locations.at(request.source_id);
      join(source_location, yijinjing::data::location::PUBLIC, request.from_time);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromSync::tag) {
      auto request = frame->data<RequestReadFromSync>();
      auto source_location = locations.at(request.source_id);
      join(source_location, yijinjing::data::location::SYNC, request.from_time);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == Deregister::tag) {
      disjoin(
          kungfu::yijinjing::data::location::make_shared(frame->data<Deregister>(), io_device_->get_locator())->uid);
    }
    next();
    return current_frame();
    //   }
    //   next();
  }
  return nullptr;
}
} // namespace kungfu::yijinjing::journal