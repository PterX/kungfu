#include "basket_instrument_store.h"
#include "io.h"

using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::node {
Napi::FunctionReference BasketInstrumentStore::constructor = {};

BasketInstrumentStore::BasketInstrumentStore(const Napi::CallbackInfo &info)
    : ObjectWrap(info), locator_(IODevice::ExtractRuntimeLocatorByIndex(info, 0)), profile_(locator_) {}

Napi::Value BasketInstrumentStore::SetAllBasketInstruments(const Napi::CallbackInfo &info) {
  if (info[0].IsArray()) {
    auto args = info[0].As<Napi::Array>();
    std::vector<BasketInstrument> basket_instruments;
    for (int i = 0; i < args.Length(); i++) {
      BasketInstrument basket_instrument = {};
      get(args.Get(i).ToObject(), basket_instrument);
      basket_instruments.push_back(basket_instrument);
    }
    try {
      profile_.remove_all<BasketInstrument>();
      for (auto basket : basket_instruments) {
        profile_.set(basket);
      }
    } catch (const std::exception &ex) {
      SPDLOG_ERROR("failed to SetAllBasketInstruments {}", ex.what());
      yijinjing::util::print_stack_trace();
      return Napi::Boolean::New(info.Env(), false);
    }
  }
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value BasketInstrumentStore::GetAllBasketInstrument(const Napi::CallbackInfo &info) {
  try {
    auto basket_instruments = profile_.get_all(BasketInstrument{});
    auto result = Napi::Array::New(info.Env(), basket_instruments.size());
    for (int i = 0; i < basket_instruments.size(); i++) {
      auto target = Napi::Object::New(info.Env());
      set(basket_instruments[i], target);
      result.Set(i, target);
    }
    return result;
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to GetAllBasketInstrument {}", ex.what());
    yijinjing::util::print_stack_trace();
    return Napi::Boolean::New(info.Env(), false);
  }
}

Napi::Value BasketInstrumentStore::RemoveAllBasketInstruments(const Napi::CallbackInfo &info) {
  try {
    profile_.remove_all<BasketInstrument>();
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to RemoveAllBasketInstruments {}", ex.what());
    yijinjing::util::print_stack_trace();
    return Napi::Boolean::New(info.Env(), false);
  }
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value BasketInstrumentStore::RemoveAllBasketInstrumentsByBasket(const Napi::CallbackInfo &info) {
  auto basket_uid = (uint32_t)GetNumber(info, 0);
  try {
    using TargetAttrPtrType = decltype(BasketInstrument::basket_uid);
    profile_.get_storage()->remove_all<BasketInstrument>(
        sqlite_orm::where(sqlite_orm::is_equal(&BasketInstrument::basket_uid, basket_uid)));
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to RemoveAllBasketInstrumentsByBasket {}", ex.what());
    yijinjing::util::print_stack_trace();
    return Napi::Boolean::New(info.Env(), false);
  }
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value BasketInstrumentStore::SetBasketInstrument(const Napi::CallbackInfo &info) {
  try {
    if (not info[0].IsObject()) {
      throw Napi::Error::New(info.Env(), "Invalid argument");
    }

    BasketInstrument basket_instrument = {};
    get(info[0].ToObject(), basket_instrument);

    profile_.set(basket_instrument);
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to SetBasketInstrument {}", ex.what());
    yijinjing::util::print_stack_trace();
    return Napi::Boolean::New(info.Env(), false);
  }
  return Napi::Boolean::New(info.Env(), true);
}

void BasketInstrumentStore::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);
  env.AddCleanupHook(cleanup);

  Napi::Function func =
      DefineClass(env, "BasketInstrumentStore",
                  {
                      InstanceMethod("setBasketInstrument", &BasketInstrumentStore::SetBasketInstrument),
                      InstanceMethod("setAllBasketInstruments", &BasketInstrumentStore::SetAllBasketInstruments),
                      InstanceMethod("getAllBasketInstrument", &BasketInstrumentStore::GetAllBasketInstrument),
                      InstanceMethod("removeAllBasketInstruments", &BasketInstrumentStore::RemoveAllBasketInstruments),
                      InstanceMethod("removeAllBasketInstrumentsByBasket",
                                     &BasketInstrumentStore::RemoveAllBasketInstrumentsByBasket),
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("BasketInstrumentStore", func);
}

Napi::Value BasketInstrumentStore::NewInstance(const Napi::Value arg) { return constructor.New({arg}); }

} // namespace kungfu::node