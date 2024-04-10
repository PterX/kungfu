#include "server.h"
#include "kungfu/yijinjing/journal/journal.h"
#include <kungfu/yijinjing/nanomsg/socket.h>

#include <fstream>
#include <unordered_map>

using namespace kungfu;
using namespace kungfu::rx;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::wingchun;
using namespace kungfu::yijinjing::nanomsg;
using namespace kungfu::yijinjing::journal;

namespace kungfu::wingchun::broker {

TestClient::TestClient(yijinjing::practice::apprentice &app) : AutoClient(app){};

void TestClient::connect(const event_ptr &event, const longfist::types::Register &register_data) {
  auto app_uid = register_data.location_uid;
  auto app_location = app_.get_location(app_uid);
  SPDLOG_DEBUG("register {}", app_location->uname);
  if (app_location->category == kungfu::longfist::enums::category::TD and should_connect_td(app_location)) {
    auto resume_time_point = get_resume_policy()->get_connect_time(app_, register_data);
    app_.request_write_to(app_.now(), app_uid);
    app_.request_read_from(app_.now(), app_uid, resume_time_point);
    SPDLOG_INFO("resume {} connection from {}", app_location->uname,
                kungfu::yijinjing::time::strftime(resume_time_point));
  }
};

} // namespace kungfu::wingchun::broker

namespace kungfu::service {

ServerConfig server::read_config(std::string filename) const {
  ServerConfig res;
  std::ifstream file(filename);
  SPDLOG_DEBUG("read_config");
  while (1) {
    if (!file.is_open()) {
      SPDLOG_ERROR("open file:{} error", filename);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }
    nlohmann::json config;
    try {
      file >> config;
    } catch (const std::exception &e) {
      SPDLOG_ERROR("parse json error!");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }
    res.address = config["address"];
    res.paths = config["paths"];
    res.thread_num = config["thread_num"];
    break;
  }
  return res;
}

server::server(locator_ptr locator, const std::string &group, const std::string &name, mode m, bool low_latency,
               const std::string &arguments)
    : apprentice(location::make_shared(m, category::SYSTEM, "service", "server", std::move(locator)), low_latency,
                 arguments),
      broker_client_(*this) {
  KUNGFU_SETUP_LOG();

  auto file_path = std::filesystem::path(std::getenv("KF_HOME")) / "server.config";
  SPDLOG_DEBUG("read file from: {}", file_path);
  ServerConfig config = read_config(file_path.generic_string());
  std::vector<std::string> paths = config.paths;
  io_network_ = std::make_shared<io_device_network_server>(config.address, paths, false);
  threadpool_ = new ThreadPool(config.thread_num);
  threadpool_->init();
}

server::~server() {}

void server::write_data(uint32_t msg_type, const char *msg, uint64_t stream_id) {
  switch (msg_type) {
  case CICC::types::AccountInfoType: {
    custom_OnInitEvent(msg, stream_id);
    break;
  }
  case CICC::types::OrderInputType: {
    custom_OnNewOrder(const_cast<char *>(msg), stream_id);
    break;
  }
  case CICC::types::OrderActionType: {
    custom_OnCancelOrder(const_cast<char *>(msg), stream_id);
    break;
  }
  default:
    break;
  }
}

void server::thread_read_data(const reader_ptr &reader, uint64_t stream_id) {
  while (reader->data_available()) {
    auto frame = reader->current_frame();
    auto type = frame->msg_type();
    auto iter = map_event_back.find(type);
    if (iter != map_event_back.end()) {
      iter->second(frame->data_address(), stream_id);
    }
    reader->next();
  }
  return;
}

/*
void server::thread_read_data(const assemble_ptr &asm_obj, uint64_t stream_id) {
  auto data_read = asm_obj->read_datas();
  uint64_t timestamp = time::now_in_nano();
  for (auto data_pair : data_read) {
    frame_header header = data_pair.first;
    int32_t type = header.msg_type;
    SPDLOG_DEBUG("type:{}",type);
    auto iter = map_event_back.find(type);
    if (iter != map_event_back.end()) {
      iter->second(data_pair.second.data(), stream_id,timestamp);
    }
  }
  return;
}
*/

bool server::custom_OnInitEvent(const char *ptr, uint64_t stream_id) {
  SPDLOG_DEBUG("custom_OnInitEvent");
  const auto *account_data = reinterpret_cast<const CICC::types::PackAccountInfo *>(ptr);
  auto &group = account_data->group;
  auto &name = account_data->name;
  SPDLOG_DEBUG("group:{} name:{}", group, name);
  auto td_location = std::make_shared<location>(mode::LIVE, category::TD, group, name, std::make_shared<locator>());
  /*
  auto asm_obj = std::make_shared<assemble>(
      std::make_shared<kungfu::yijinjing::data::locator>(),
      get_mode_name(mode::LIVE), get_category_name(category::TD), group, name);
  asm_obj->join_channel(get_home(), td_location->location_uid, 0);
*/

  auto reader = std::make_shared<kungfu::yijinjing::journal::reader>(true, false, std::make_shared<bus>(false));
  auto now = time::now_in_nano();
  reader->join(td_location, get_home_uid(), now);
  reader->join(get_home(), td_location->location_uid, now);
  stream_reader_map.try_emplace(stream_id, reader);

  if (has_writer(td_location->location_uid)) {
    auto writer = get_writer(td_location->location_uid);
    stream_writer_map.try_emplace(stream_id, writer);
    SPDLOG_DEBUG("add writer for stream:{}", stream_id);
    // maybe need to do something with td
    // reader_->join(td_location,get_home_uid(),event->gen_time());   no need
    // join, should use assmble
  } else {
    // should write error to data;
    SPDLOG_ERROR("td {}_{} not exist!", group, name);
  }
  SPDLOG_DEBUG("end custom_OnInitEvent");
  return true;
}

bool server::custom_OnNewOrder(const char *ptr, uint64_t stream_id) {
  auto *remote_data = reinterpret_cast<const CICC::types::PackOrderInput *>(ptr);
  auto remote_input = remote_data->data;
  std::string instrument_id = remote_input.instrument_id;
  std::string exchange_id = remote_input.exchange_id;

  auto instrument_type = get_instrument_type(exchange_id, instrument_id);
  if (instrument_type == InstrumentType::Unknown) {
    SPDLOG_ERROR("unsupported instrument type {} of {}.{}", str_from_instrument_type(instrument_type), instrument_id,
                 exchange_id);
    return 0;
  }

  if (!stream_writer_map.contains(stream_id)) {
    SPDLOG_ERROR("do not have writer for steam:{}", stream_id);
    return false;
  }
  auto writer = stream_writer_map.find(stream_id)->second;
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderInput &input = writer->open_data<OrderInput>(time::now_in_nano());
  memcpy(&input, &(remote_data->data), sizeof(OrderInput));
  input.order_id = writer->current_frame_uid();
  input.insert_time = time::now_in_nano();
  writer->close_data();
  request_order_map.try_emplace(std::make_pair(stream_id, remote_input.request_id), input.order_id);
  return true;
}

bool server::custom_OnCancelOrder(const char *ptr, uint64_t stream_id) {
  auto *remote_data = reinterpret_cast<const CICC::types::PackOrderAction *>(ptr);
  auto remote_action = remote_data->data;

  std::pair<uint64_t, uint64_t> stream_request_pair = std::make_pair(stream_id, remote_action.order_action_id);
  if (!request_order_map.contains(stream_request_pair)) {
    // TODO:should write OrderActionError
    SPDLOG_ERROR("do not have order for stream:{} request:{}", stream_id, remote_action.order_action_id);
    return false;
  }

  if (!stream_writer_map.contains(stream_id)) {
    // TODO:should write OrderActionError
    SPDLOG_ERROR("do not have writer for stream:{}", stream_id);
    return false;
  }

  auto writer = stream_writer_map.find(stream_id)->second;
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderAction &action = writer->open_data<OrderAction>(time::now_in_nano());
  memcpy(&action, &(remote_data->data), sizeof(OrderAction));
  action.order_id = request_order_map.at(stream_request_pair);
  action.insert_time = time::now_in_nano();
  writer->close_data();
  return true;
}

bool server::custom_OnQryAlgoParentOrder(const char *ptr) { return true; }

void server::deal_msg(const rx::subscriber<event_ptr> &sb) {
  for (auto stream : io_network_->get_stream_manager()->get_all_streams()) {
    uint64_t stream_id = stream.first;
    auto reader = io_network_->get_stream_manager()->get_reader(stream_id);
    int count = 0;
    while (reader != nullptr and reader->data_available() and count < 100) {
      const char *data = reader->current_frame()->data_as_bytes();
      write_data(reinterpret_cast<const uint32_t &>(*data), data, stream_id);
      reader->next();
      ++count;
    }
    //    auto msgs = io_network_->get_stream_manager()->get_notice(stream.first);
    //    for (auto msg : msgs) {
    //      uint32_t messageType;
    //      std::memcpy(&messageType, msg.data(), sizeof(uint32_t));
    //      write_data(messageType, msg, stream.first);
    //    }
  }
}

void server::submit_read_read_assemble() {
  for (auto &item : stream_reader_map) {
    auto it = stream_task_map.find(item.first);
    if (it != stream_task_map.end() && it->second.valid() &&
        it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      continue;
    }
    stream_task_map[item.first] =
        threadpool_->submit(std::mem_fn(&server::thread_read_data), this, item.second, item.first);
  }
}

bool server::drain(const rx::subscriber<event_ptr> &sb) {
  bool bypass = io_device_->is_lazy() and is_low_latency();
  deal_msg(sb);
  deal_notice(bypass, true, sb);
  submit_read_read_assemble();
  // loop_time[loop_num<max_num?loop_num++:max_num-1] = time::now_in_nano();
  for (std::size_t step_count = 0;                                                             //
       live_ and reader_->data_available() and (step_limit_ == 0 || step_count < step_limit_); //
       step_count++) {
    // data_time[data_num<max_num?data_num++:max_num-1] = time::now_in_nano();
    // SPDLOG_DEBUG("time:{}",time::now_in_nano());
    deal_msg(sb);
    deal_notice(io_device_->is_lazy(), false, sb);
    submit_read_read_assemble();
    const frame_ptr frame = reader_->current_frame();
    io_device_->get_bus()->set_trigger_frame(frame);
    if (frame->gen_time() <= end_time_) {
      int64_t frame_time = frame->gen_time();
      if (frame_time > now_) {
        now_ = frame_time;
      }
      if (is_reactable(frame)) {
        sb.on_next(frame);
      }
      on_frame();
      reader_->next();
      cleanup_reader_disjoin();
    } else {
      SPDLOG_INFO("reached journal end {}", time::strftime(frame->gen_time()));
      return false;
    }
  }
  if (get_io_device()->get_home()->mode != mode::LIVE and not reader_->data_available()) {
    SPDLOG_INFO("reached journal end {}", time::strftime(now()));
    return false;
  }
  return true;
}

/*
bool server::drain(const rx::subscriber<event_ptr> &sb) {
  deal_msg(sb);
  hero::drain(sb);
  for (auto &item : stream_reader_map) {
    auto it = stream_task_map.find(item.first);
    if (it != stream_task_map.end() && it->second.valid() &&
        it->second.wait_for(std::chrono::seconds(0)) !=
            std::future_status::ready) {
      continue;
    }
    stream_task_map[item.first] = threadpool_->submit(
        std::mem_fn(&server::thread_read_data), this, item.second, item.first);
  }
  return true;
};
*/

void server::on_exit() { SPDLOG_DEBUG("exit!"); }

void server::on_start() {
  broker_client_.on_start(events_);
  SPDLOG_DEBUG("on_start  thread_id:{}", std::this_thread::get_id());
  SPDLOG_DEBUG("end on_start");
}
} // namespace kungfu::service
