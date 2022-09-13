//
// Created by Keren Dong on 2020/1/1.
//

#include "journal.h"
#include "io.h"
#include "operators.h"
#include <cmath>

using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::longfist::types;

namespace kungfu::node {
int64_t GetTimestamp(Napi::Value arg) {
  if (arg.IsNumber()) {
    return arg.ToNumber().Int32Value();
  }
  if (arg.IsBigInt()) {
    bool lossless;
    return arg.As<Napi::BigInt>().Int64Value(&lossless);
  }
  throw yijinjing_error("timestamp argument must be bigint");
}

Napi::FunctionReference Frame::constructor = {};

Frame::Frame(const Napi::CallbackInfo &info) : ObjectWrap(info) {}

void Frame::SetFrame(yijinjing::journal::frame_ptr frame, std::string source_name, std::string dest_name) {
  frame_ = std::move(frame);
  source_name_ = source_name;
  dest_name_ = dest_name;
}

Napi::Value Frame::DataLength(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), frame_->data_length());
}

Napi::Value Frame::GenTime(const Napi::CallbackInfo &info) { return Napi::BigInt::New(info.Env(), frame_->gen_time()); }

Napi::Value Frame::TriggerTime(const Napi::CallbackInfo &info) {
  return Napi::BigInt::New(info.Env(), frame_->trigger_time());
}

Napi::Value Frame::MsgType(const Napi::CallbackInfo &info) { return Napi::Number::New(info.Env(), frame_->msg_type()); }

Napi::Value Frame::Source(const Napi::CallbackInfo &info) { return Napi::Number::New(info.Env(), frame_->source()); }

Napi::Value Frame::Dest(const Napi::CallbackInfo &info) { return Napi::Number::New(info.Env(), frame_->dest()); }

Napi::Value Frame::SourceName(const Napi::CallbackInfo &info) { return Napi::String::New(info.Env(), source_name_); }

Napi::Value Frame::DestName(const Napi::CallbackInfo &info) { return Napi::String::New(info.Env(), dest_name_); }

Napi::Value Frame::Data(const Napi::CallbackInfo &info) {
  auto ret = Napi::String::New(info.Env(), "");
  boost::hana::for_each(longfist::AllTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (frame_->msg_type() == DataType::tag) {
      ret = Napi::String::New(info.Env(), frame_->data<DataType>().to_string());
    }
  });
  return ret;
}

Napi::Value Frame::StringMsgType(const Napi::CallbackInfo &info) {
  auto ret = Napi::String::New(info.Env(), "");
  boost::hana::for_each(longfist::AllTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (frame_->msg_type() == DataType::tag) {
      ret = Napi::String::New(info.Env(), boost::hana::first(it).c_str());
    }
  });
  return ret;
}

void Frame::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Frame",
                                    {
                                        InstanceMethod("dataLength", &Frame::DataLength),       //
                                        InstanceMethod("genTime", &Frame::GenTime),             //
                                        InstanceMethod("triggerTime", &Frame::TriggerTime),     //
                                        InstanceMethod("msgType", &Frame::MsgType),             //
                                        InstanceMethod("stringMsgType", &Frame::StringMsgType), //
                                        InstanceMethod("source", &Frame::Source),               //
                                        InstanceMethod("dest", &Frame::Dest),                   //
                                        InstanceMethod("sourceName", &Frame::SourceName),       //
                                        InstanceMethod("destName", &Frame::DestName),           //
                                        InstanceMethod("data", &Frame::Data)                    //
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("Frame", func);
}

Napi::Value Frame::NewInstance(const Napi::Value arg) { return constructor.New({arg}); }

bool b;
Napi::FunctionReference Reader::constructor = {};
Reader::Reader(const Napi::CallbackInfo &info)
    : ObjectWrap(info), reader(true), io_device_(std::make_shared<io_device>(GetLocation(info), true, true)),
      begin_time_(info[4].As<Napi::BigInt>().Int64Value(&b)), end_time_(info[5].As<Napi::BigInt>().Int64Value(&b)) {
  SPDLOG_INFO("begin_time_ {} end_time_ {} b {}", begin_time_, end_time_, b);
  if (true) {
    auto uid_str = fmt::format("{:08x}", io_device_->get_home()->uid);
    auto master_cmd_location =
        location::make_shared(kungfu::longfist::enums::mode::LIVE, kungfu::longfist::enums::category::SYSTEM, "master",
                              uid_str, io_device_->get_locator());
    auto master_home_location =
        location::make_shared(kungfu::longfist::enums::mode::LIVE, kungfu::longfist::enums::category::SYSTEM, "master",
                              "master", io_device_->get_locator());
    bool is_master = io_device_->get_home()->name.compare("master") == 0;
    if (!is_master) {
      join(master_cmd_location, io_device_->get_home()->uid, begin_time_);
    }
    join(master_home_location, location::PUBLIC, begin_time_);
  }
  if (true) {
    for (auto dest_id : io_device_->get_locator()->list_location_dest(io_device_->get_home())) {
      join(io_device_->get_home(), dest_id, begin_time_);
    }
  }
}

location_ptr Reader::GetLocation(const Napi::CallbackInfo &info) {
  kungfu::longfist::enums::mode m = (kungfu::longfist::enums::mode)(info[0].ToNumber().Uint32Value());
  kungfu::longfist::enums::category c = (kungfu::longfist::enums::category)(info[1].ToNumber().Uint32Value());
  std::string group = info[2].ToString().Utf8Value();
  std::string name = info[3].ToString().Utf8Value();
  return std::make_shared<location>(m, c, group, name, GetDefaultRuntimeLocator());
}

Napi::Value Reader::ToString(const Napi::CallbackInfo &info) { return Napi::String::New(info.Env(), "Reader.js"); }

Napi::Value Reader::CurrentFrame(const Napi::CallbackInfo &info) {
  auto frame = Frame::NewInstance(info.This());
  std::string s;
  std::string d;
  auto c_frame = current_frame();
  if (c_frame->dest() == location::PUBLIC) {
    d = "public";
  } else if (locations_.find(c_frame->dest()) != locations_.end()) {
    d = locations_.at(c_frame->dest())->uname;
  }
  if (locations_.find(c_frame->source()) != locations_.end()) {
    s = locations_.at(c_frame->source())->uname;
  }
  Napi::ObjectWrap<Frame>::Unwrap(frame.As<Napi::Object>())->SetFrame(current_frame(), s, d);
  return frame;
}

Napi::Value Reader::SeekToTime(const Napi::CallbackInfo &info) {
  seek_to_time(GetTimestamp(info[0]));
  return {};
}

Napi::Value Reader::DataAvailable(const Napi::CallbackInfo &info) {
  return Napi::Boolean::New(info.Env(), data_available());
}

Napi::Value Reader::Next(const Napi::CallbackInfo &info) {
  if (data_available() && current_frame()->gen_time() <= end_time_) {
    // if (current_frame()->gen_time() >= begin_time_) {
    if (locations_.empty()) {
      for (auto location : io_device_->get_home()->locator->list_locations(".*", ".*", ".*", ".*")) {
        locations_.insert_or_assign(location->uid, location);
      }
    }
    auto frame = current_frame();
    auto dest_name = frame->dest() == location::PUBLIC ? "public" : locations_.at(frame->dest())->uname;
    bool type_found = false;
    boost::hana::for_each(kungfu::longfist::AllTypes, [&](auto type) {
      using DataType = typename decltype(+boost::hana::second(type))::type;
      if (frame->msg_type() == DataType::tag) {
        SPDLOG_INFO("Next {} {} {} {} {} {}", time::strftime(frame->gen_time(), "%T.%N"),
                    time::strftime(frame->trigger_time(), "%T.%N"), locations_.at(frame->source())->uname, dest_name,
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
      auto source_location = locations_.at(request.source_id);
      join(source_location, io_device_->get_home()->uid, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromPublic::tag) {
      auto request = frame->data<RequestReadFromPublic>();
      auto source_location = locations_.at(request.source_id);
      join(source_location, location::PUBLIC, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromSync::tag) {
      auto request = frame->data<RequestReadFromSync>();
      auto source_location = locations_.at(request.source_id);
      join(source_location, location::SYNC, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == Deregister::tag) {
      disjoin(location::make_shared(frame->data<Deregister>(), io_device_->get_locator())->uid);
    }
    // return Napi::Boolean::New(info.Env(), true);
    auto f = CurrentFrame(info);
    next();
    return f;
    // }
  }
  return info.Env().Null();
}

Napi::Value Reader::Join(const Napi::CallbackInfo &info) {
  auto category = longfist::enums::get_category_by_name(info[0].As<Napi::String>().Utf8Value());
  auto group = info[1].As<Napi::String>().Utf8Value();
  auto name = info[2].As<Napi::String>().Utf8Value();
  auto mode = longfist::enums::get_mode_by_name(info[3].As<Napi::String>().Utf8Value());
  uint32_t dest_id = info[4].As<Napi::Number>().Int32Value();
  auto from_time = GetTimestamp(info[5]);
  join(std::make_shared<location>(mode, category, group, name, io_device_->get_home()->locator), dest_id, from_time);
  return {};
}

Napi::Value Reader::Disjoin(const Napi::CallbackInfo &info) {
  uint32_t dest_id = info[0].As<Napi::Number>().Int32Value();
  disjoin(dest_id);
  return {};
}

Napi::Value Reader::Run(const Napi::CallbackInfo &info) {
  int32_t limit = 0;
  Napi::Function cb = info[0].As<Napi::Function>();
  if (info.Length() > 1) {
    limit = info[1].ToNumber().Int32Value();
  }
  if (locations_.empty()) {
    for (auto location : io_device_->get_home()->locator->list_locations(".*", ".*", ".*", ".*")) {
      locations_.insert_or_assign(location->uid, location);
    }
  }
  int32_t count = 0;
  while ((limit <= 0 || count++ < limit) && data_available() && current_frame()->gen_time() <= end_time_) {
    // if (current_frame()->gen_time() >= begin_time_) {
    auto frame = current_frame();
    auto dest_name = frame->dest() == location::PUBLIC ? "public" : locations_.at(frame->dest())->uname;
    bool type_found = false;
    boost::hana::for_each(kungfu::longfist::AllTypes, [&](auto type) {
      using DataType = typename decltype(+boost::hana::second(type))::type;
      if (frame->msg_type() == DataType::tag) {
        SPDLOG_INFO("Next {} {} {} {} {} {}", time::strftime(frame->gen_time(), "%T.%N"),
                    time::strftime(frame->trigger_time(), "%T.%N"), locations_.at(frame->source())->uname, dest_name,
                    DataType::type_name.c_str(), frame->data<DataType>().to_string());
        type_found = true;
      }
    });
    if (not type_found) {
      auto location_uname = current_page()->get_location()->uname;
      auto dest_id = current_page()->get_dest_id();
      SPDLOG_ERROR("{}/{:08x} msg_type {} not found", location_uname, dest_id, frame->msg_type());
      cb.Call({info.Env().Null()});
      return {};
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFrom::tag) {
      auto request = frame->data<RequestReadFrom>();
      auto source_location = locations_.at(request.source_id);
      join(source_location, io_device_->get_home()->uid, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromPublic::tag) {
      auto request = frame->data<RequestReadFromPublic>();
      auto source_location = locations_.at(request.source_id);
      join(source_location, location::PUBLIC, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == RequestReadFromSync::tag) {
      auto request = frame->data<RequestReadFromSync>();
      auto source_location = locations_.at(request.source_id);
      join(source_location, location::SYNC, begin_time_);
    }
    if (frame->dest() == io_device_->get_home()->uid and frame->msg_type() == Deregister::tag) {
      disjoin(location::make_shared(frame->data<Deregister>(), io_device_->get_locator())->uid);
    }
    auto node_frame = CurrentFrame(info);
    cb.Call({node_frame});
    // }
    next();
  }
  cb.Call({info.Env().Null()});
  return {};
}

void Reader::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Reader",
                                    {
                                        InstanceMethod("toString", &Reader::ToString),           //
                                        InstanceMethod("currentFrame", &Reader::CurrentFrame),   //
                                        InstanceMethod("seekToTime", &Reader::SeekToTime),       //
                                        InstanceMethod("dataAvailable", &Reader::DataAvailable), //
                                        InstanceMethod("next", &Reader::Next),                   //
                                        InstanceMethod("join", &Reader::Join),                   //
                                        InstanceMethod("disjoin", &Reader::Disjoin),             //
                                        InstanceMethod("run", &Reader::Run),                     //
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Reader", func);
}

Napi::Value Reader::NewInstance(const Napi::Value arg) { return constructor.New({arg}); }

Napi::FunctionReference Assemble::constructor = {};

Assemble::Assemble(const Napi::CallbackInfo &info) : ObjectWrap(info), assemble(ExtractLocator(info)) {}

Napi::Value Assemble::CurrentFrame(const Napi::CallbackInfo &info) {
  auto frame = Frame::NewInstance(info.This());
  Napi::ObjectWrap<Frame>::Unwrap(frame.As<Napi::Object>())->SetFrame(current_frame());
  return frame;
}

Napi::Value Assemble::SeekToTime(const Napi::CallbackInfo &info) {
  if (not IsValid(info, 0, &Napi::Value::IsBigInt)) {
    return {};
  }
  auto time = GetBigInt(info, 0);
  for (auto &reader : readers_) {
    reader->seek_to_time(time);
  }
  return {};
}

Napi::Value Assemble::DataAvailable(const Napi::CallbackInfo &info) {
  return Napi::Boolean::New(info.Env(), data_available());
}

Napi::Value Assemble::Next(const Napi::CallbackInfo &info) {
  next();
  return {};
}

Napi::Value Assemble::Get_sessions(const Napi::CallbackInfo &info) {
  uint32_t uid = 0;
  bool filter(false);
  if (info.Length() == 1 && info[0].IsObject()) {
    uid = info[0].ToObject().Get("location_uid").ToNumber().Uint32Value();
    filter = true;
  }
  std::vector<kungfu::longfist::types::Session> sessions = get_sessions();
  std::vector<kungfu::longfist::types::Session> session_ret;
  size_t session_size = sessions.size();
  for (int i = 0; i < session_size; i++) {
    if (!filter || sessions[i].location_uid == uid) {
      session_ret.push_back(sessions[i]);
    }
  }
  size_t session_ret_size = session_ret.size();
  if (session_ret_size <= 0) {
    return {};
  }
  auto result = Napi::Array::New(info.Env(), session_ret_size);
  for (int i = 0; i < session_ret_size; i++) {
    auto target = Napi::Object::New(info.Env());
    set(session_ret[i], target);
    result.Set(i, target);
  }
  return result;
}

Napi::Value Assemble::Get_reader(const Napi::CallbackInfo &info) {
  std::vector<kungfu::longfist::types::Session> sessions = get_sessions();
  size_t session_size = sessions.size();
  uint32_t index = info[0].ToNumber().Uint32Value();
  if (session_size <= index) {
    throw Napi::Error::New(info.Env(), "index greater than session size");
  }
  int64_t t_begin = 0;
  int64_t t_end = 0;
  bool bRet(false);
  if (info.Length() > 1) {
    t_begin = info[1].As<Napi::BigInt>().Int64Value(&bRet);
    if (info.Length() > 2) {
      t_end = info[2].As<Napi::BigInt>().Int64Value(&bRet);
    }
  }
  // SPDLOG_INFO("sessions[index].mode {} sessions[index].category {} sessions[index].group {} sessions[index].name
  // {}",int(sessions[index].mode), int(sessions[index].category), sessions[index].group, sessions[index].name);
  auto node_mode = Napi::Number::New(info.Env(), int(sessions[index].mode));
  auto node_category = Napi::Number::New(info.Env(), int(sessions[index].category));
  auto node_group = Napi::String::New(info.Env(), sessions[index].group);
  auto node_name = Napi::String::New(info.Env(), sessions[index].name);
  auto begin_time = Napi::BigInt::New(info.Env(), t_begin > 0 ? t_begin : sessions[index].begin_time);
  auto end_time = Napi::BigInt::New(info.Env(), t_end > 0 ? t_end : std::abs(sessions[index].end_time));
  auto reader = Reader::constructor.New({node_mode, node_category, node_group, node_name, begin_time, end_time});
  return reader;
}

void Assemble::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Assemble",
                                    {
                                        InstanceMethod("get_sessions", &Assemble::Get_sessions),
                                        InstanceMethod("get_reader", &Assemble::Get_reader),
                                        InstanceMethod("currentFrame", &Assemble::CurrentFrame),   //
                                        InstanceMethod("seekToTime", &Assemble::SeekToTime),       //
                                        InstanceMethod("dataAvailable", &Assemble::DataAvailable), //
                                        InstanceMethod("next", &Assemble::Next),                   //
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Assemble", func);
}

std::vector<locator_ptr> Assemble::ExtractLocator(const Napi::CallbackInfo &info) {
  if (not IsValid(info, 0, &Napi::Value::IsArray)) {
    throw Napi::Error::New(info.Env(), "Invalid locators argument");
  }
  std::vector<locator_ptr> result = {};
  auto locators = info[0].As<Napi::Array>();
  for (int i = 0; i < locators.Length(); i++) {
    // result.push_back(IODevice::GetLocator(locators, i));
    continue;
  }
  return result;
}
} // namespace kungfu::node
