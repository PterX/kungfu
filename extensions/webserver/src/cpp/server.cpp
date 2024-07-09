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
using namespace kungfu::yijinjing::webserver;

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
  web_agent_ = std::make_shared<http_server>(config.address);
  for (const auto &path : paths) {
    //    auto http_server_ptr = std::dynamic_pointer_cast<http_server>(web_agent_);
    //    if (http_server_ptr) {
    //    http_server_ptr->add_websocket(path, false, true);
    //    } else {
    //      SPDLOG_ERROR("server pointer cast error");
    //    }
    web_agent_->add_websocket(path, false, true);
  }
  // threadpool_ = new ThreadPool(config.thread_num);
  // threadpool_->init();
}

server::~server() {}

/*
void server::write_data(uint32_t msg_type, const char *msg, uint64_t stream_id, uint64_t gen_time) {
  SPDLOG_DEBUG("write_data");
  switch (msg_type) {
  case CICC::types::AccountInfoType: {
    custom_OnInitEvent(msg, stream_id);
    break;
  }
  case CICC::types::OrderInputType: {
    custom_OnNewOrder(const_cast<char *>(msg), stream_id,gen_time);
    break;
  }
  case CICC::types::OrderActionType: {
    custom_OnCancelOrder(const_cast<char *>(msg), stream_id);
    break;
  }
  case CICC::types::ReqType: {
    auto *round_req = reinterpret_cast<const CICC::types::PackRoundReq *>(msg);
    uint32_t limit = round_req->limit;
    stream_limit_map_.emplace(stream_id, limit);
    SPDLOG_DEBUG("stream_id:{} limit:{}",stream_id, limit);
    if(stream_workers_.contains(stream_id)){
        stream_workers_.at(stream_id)->notify();
    }
    else{
        SPDLOG_ERROR("couldn't find cv for stream:{}",stream_id);
    }
    break;
  }
  default:
    break;
  }
  SPDLOG_DEBUG("finish write_data");
  return;
}


void server::thread_read_data(const location_ptr &td_location, ThreadWorker_ptr worker) {
  auto stream = worker->get_stream();
  auto stream_id = stream->get_stream_id();
  SPDLOG_INFO("stream {} thread_read_data", stream_id);
  int limit = stream_limit_map_.contains(stream_id) ? stream_limit_map_.at(stream_id) : 100;
  int nums = 0;
  auto reader = std::make_shared<kungfu::yijinjing::journal::reader>(true, false, std::make_shared<bus>(false));
  auto now = time::now_in_nano();
  reader->join(td_location, get_home_uid(), now);
  reader->join(get_home(), td_location->location_uid, now);

  while (worker->is_live()) {
    while (reader->data_available() && nums < limit) {
      auto frame = reader->current_frame();
      auto type = frame->msg_type();
      auto iter = map_event_back.find(type);
      if (iter != map_event_back.end()) {
        iter->second(frame->data_address(), stream);
      }
      reader->next();
      nums++;
    }
    CICC::types::PackReqEnd data_send{};
    stream->stream_send((char *)(&data_send), sizeof(CICC::types::PackReqEnd));
    nums = 0;
    worker->wait();
  }
  return;
}

void server::thread_send_data(const location_ptr &td_location, ThreadWorker_ptr worker) {
  auto stream = worker->get_stream();
  auto stream_id = stream->get_stream_id();
  SPDLOG_INFO("stream {} thread_send_data", stream_id);
  auto reader = std::make_shared<kungfu::yijinjing::journal::reader>(true, false, std::make_shared<bus>(false));
  auto now = time::now_in_nano();
  reader->join(td_location, get_home_uid(), now);
  reader->join(get_home(), td_location->location_uid, now);

  while (worker->is_live()) {
    while (reader->data_available()){
      auto frame = reader->current_frame();
      auto type = frame->msg_type();
      switch (type) {
      case OrderInput::tag: {
        // auto data = const_cast<OrderInput *>(frame->data_address());
        CICC::types::PackOrderInput data_send;
        memcpy(&data_send.data, frame->data_address(), sizeof(OrderInput));
        data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
        stream->stream_send((char *)(&data_send), sizeof(CICC::types::PackOrderInput));
        // web_agent_->publish((char *)(&data_send), sizeof(CICC::types::PackOrderInput), stream_id);
        break;
      }
      case Order::tag: {
        // auto data = const_cast<Order *>(frame->data_address());
        CICC::types::PackOrder data_send;
        memcpy(&data_send.data, frame->data_address(), sizeof(Order));
        data_send.data.parent_id = kungfu::yijinjing::time::now_in_nano();
        // web_agent_->publish((char *)(&data_send), sizeof(CICC::types::PackOrder), stream_id);
        stream->stream_send((char *)(&data_send), sizeof(CICC::types::PackOrder));
        break;
      }
      case Trade::tag: {
        // auto data = const_cast<Trade *>(frame->data_address());
        CICC::types::PackTrade data_send;
        memcpy(&data_send.data, frame->data_address(), sizeof(Trade));
        data_send.data.parent_order_id = kungfu::yijinjing::time::now_in_nano();
        // web_agent_->publish((char *)(&data_send), sizeof(CICC::types::PackTrade), stream_id);
        stream->stream_send((char *)(&data_send), sizeof(CICC::types::PackTrade));
        break;
      }

      default:
        break;
      }
      // auto iter = map_event_back.find(type);
      reader->next();
      }
  }
  return;
}

bool server::custom_OnInitEvent(const char *ptr, uint64_t stream_id) {
  const auto *account_data = reinterpret_cast<const CICC::types::PackAccountInfo *>(ptr);
  auto &group = account_data->group;
  auto &name = account_data->name;
  auto &method = account_data->method;
  SPDLOG_DEBUG("group:{} name:{}", group, name);
  auto td_location = std::make_shared<location>(mode::LIVE, category::TD, group, name, std::make_shared<locator>());

  if (!has_writer(td_location->location_uid)) {
    SPDLOG_ERROR("td {}_{} not exist!", group, name);
    return false;
  }

  auto writer = get_writer(td_location->location_uid);
  stream_writer_map_.try_emplace(stream_id, writer);
  SPDLOG_DEBUG("add writer for stream:{}", stream_id);

  if (method == CICC::enums::Method::direct) {
    // 创建worker, 每个线程一个worker, 里面有属于每一个单独线程的cv和锁
    stream_workers_.insert_or_assign(stream_id,
std::make_shared<ThreadWorker>(web_agent_->get_stream_by_id(stream_id))); stream_thread_map_.try_emplace(stream_id,
std::make_shared<std::thread>(&server::thread_send_data, this, td_location, stream_workers_.at(stream_id))); } else if
(method == CICC::enums::Method::round) {

    // 创建worker, 每个线程一个worker, 里面有属于每一个单独线程的cv和锁
    stream_workers_.insert_or_assign(stream_id,
std::make_shared<ThreadWorker>(web_agent_->get_stream_by_id(stream_id))); stream_thread_map_.try_emplace(stream_id,
std::make_shared<std::thread>(&server::thread_read_data, this, td_location, stream_workers_.at(stream_id)));

    // auto reader = std::make_shared<kungfu::yijinjing::journal::reader>(true, false, std::make_shared<bus>(false));
    // auto now = time::now_in_nano();
    // reader->join(td_location, get_home_uid(), now);
    // reader->join(get_home(), td_location->location_uid, now);
    // stream_reader_map_.try_emplace(stream_id, reader);

  } else {
    return false;
  }
  return true;
}

bool server::custom_OnNewOrder(const char *ptr, uint64_t stream_id,uint64_t gen_time) {
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

  if (!stream_writer_map_.contains(stream_id)) {
    SPDLOG_ERROR("do not have writer for steam:{}", stream_id);
    return false;
  }
  auto writer = stream_writer_map_.find(stream_id)->second;
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderInput &input = writer->open_data<OrderInput>(time::now_in_nano());
  memcpy(&input, &(remote_data->data), sizeof(OrderInput));
  input.order_id = writer->current_frame_uid();
  input.insert_time = time::now_in_nano();
  input.block_id = gen_time;
  writer->close_data();
  request_order_map_.try_emplace(std::make_pair(stream_id, remote_input.request_id), input.order_id);
  return true;
}

bool server::custom_OnCancelOrder(const char *ptr, uint64_t stream_id) {
  auto *remote_data = reinterpret_cast<const CICC::types::PackOrderAction *>(ptr);
  auto remote_action = remote_data->data;

  std::pair<uint64_t, uint64_t> stream_request_pair = std::make_pair(stream_id, remote_action.order_action_id);
  if (!request_order_map_.contains(stream_request_pair)) {
    // TODO:should write OrderActionError
    SPDLOG_ERROR("do not have order for stream:{} request:{}", stream_id, remote_action.order_action_id);
    return false;
  }

  if (!stream_writer_map_.contains(stream_id)) {
    // TODO:should write OrderActionError
    SPDLOG_ERROR("do not have writer for stream:{}", stream_id);
    return false;
  }

  auto writer = stream_writer_map_.find(stream_id)->second;
  page_ptr page = writer->get_current_page(); // prevent that page released after close_data
  OrderAction &action = writer->open_data<OrderAction>(time::now_in_nano());
  memcpy(&action, &(remote_data->data), sizeof(OrderAction));
  action.order_id = request_order_map_.at(stream_request_pair);
  action.insert_time = time::now_in_nano();
  writer->close_data();
  return true;
}

bool server::custom_OnQryAlgoParentOrder(const char *ptr) { return true; }
*/
void server::deal_msg(const rx::subscriber<event_ptr> &sb) {
  // auto manager = web_agent_->get_stream_manager();
  // SPDLOG_DEBUG("before reader");
  //   auto &reader = web_agent_->get_stream_manager()->get_reader();
  // SPDLOG_DEBUG("after reader");
  int count = 0;
  SPDLOG_DEBUG("before on_frame");
  web_agent_->on_frame();
  SPDLOG_DEBUG("after on_frame");
  while (web_agent_->data_available() and count < 100) {
    const char *data = web_agent_->current_frame()->data_as_bytes();
    uint64_t gen_time = web_agent_->current_frame()->gen_time();
    uint64_t msg_type = web_agent_->current_frame()->msg_type();
    uint64_t stream_id = web_agent_->current_frame()->stream_id();
    // uint64_t stream_id = manager->get_stream_id(location_uid);
    SPDLOG_DEBUG("after get_stream_id:{} ", stream_id);
    if (msg_type == NngDisconnect::tag) {
      // manager->remove_stream(manager->get_stream_id(location_uid));
      SPDLOG_DEBUG("get NngDisconnect");
      stream_workers_.erase(stream_id);
      stream_thread_map_.erase(stream_id);
      continue;
    }
    SPDLOG_DEBUG("before write_data: pack_type:{} frame_type:{}", reinterpret_cast<const uint32_t &>(*data), msg_type);
    // write_data(reinterpret_cast<const uint32_t &>(*data), data, stream_id,gen_time);
    SPDLOG_DEBUG("after write_data");
    web_agent_->next();
    ++count;
  }
  // SPDLOG_DEBUG("finish deal_msg");
  return;
}

bool server::drain(const rx::subscriber<event_ptr> &sb) {
  bool bypass = io_device_->is_lazy() and is_low_latency();
  deal_msg(sb);
  deal_notice(bypass, true, sb);
  for (std::size_t step_count = 0;
       live_ and reader_->data_available() and (step_limit_ == 0 || step_count < step_limit_); step_count++) {
    deal_msg(sb);
    deal_notice(io_device_->is_lazy(), false, sb);
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

void server::on_exit() { SPDLOG_DEBUG("exit!"); }

void server::on_start() {
  broker_client_.on_start(events_);

  SPDLOG_DEBUG("end on_start");
  return;
}
} // namespace kungfu::service
