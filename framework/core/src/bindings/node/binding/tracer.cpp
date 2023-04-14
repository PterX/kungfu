#include "io.h"
#include "journal.h"

using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::longfist::types;

namespace kungfu::node {

Napi::FunctionReference Tracer::constructor = {};

Tracer::Tracer(const Napi::CallbackInfo &info)
    : ObjectWrap(info),                                                                           //
      tracer(IODevice::ExtractLocation(info, 0, IODevice::ExtractRuntimeLocatorByIndex(info, 1)), //
             GetBool(info, 2),                                                                    //
             GetBool(info, 3),                                                                    //
             GetBigInt(info, 4),                                                                  //
             GetBigInt(info, 5)) {}

Tracer::~Tracer() { SPDLOG_INFO("Tracer destructor"); }

Napi::Value Tracer::DataAvailable(const Napi::CallbackInfo &info) {
  return Napi::Boolean::New(Env(), data_available());
}

Napi::Value Tracer::CurrentFrame(const Napi::CallbackInfo &info) {
  auto frame = Frame::NewInstance(info.This());
  Napi::ObjectWrap<Frame>::Unwrap(frame.As<Napi::Object>())->SetFrame(current_frame());
  return frame;
}

void Tracer::Next(const Napi::CallbackInfo &info) { next(); }

void Tracer::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Tracer",
                                    {
                                        InstanceMethod("currentFrame", &Tracer::CurrentFrame),   //
                                        InstanceMethod("dataAvailable", &Tracer::DataAvailable), //
                                        InstanceMethod("next", &Tracer::Next),                   //
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Tracer", func);
}

Napi::Value Tracer::NewInstance(const Napi::Value arg) { return constructor.New({arg}); }

} // namespace kungfu::node