#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::strategy;
using namespace kungfu::yijinjing::data;
int i = 0;
KUNGFU_MAIN_STRATEGY(KungfuStrategy101) {
public:
  KungfuStrategy101() = default;
  ~KungfuStrategy101() = default;

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->get_arguments());
    //    context->add_account("sim", "fill");
    context->subscribe("sim", {"600000"}, {"SSE"});
    context->subscribe("xtp", {"600009"}, {"SSE"});
    // context->subscribe_operator("bar", "my-bar");
  }

  void post_start(Context_ptr & context) override {
    SPDLOG_INFO("strategy started");
    // auto &runtime = dynamic_cast<RuntimeContext &>(*context);
    // auto &bookkeeper = runtime.get_bookkeeper();
    // auto &books = bookkeeper.get_books();
    // for (const auto &pair : books) {
    //   auto &book = pair.second;
    //   SPDLOG_INFO("book asset: {}", book->asset.to_string());
    // }
    //    auto l_ptr = location::make_shared(mode::LIVE, category::MD, "sim", "sim", std::make_shared<locator>());
    //    kungfu::yijinjing::journal::assemble asb(l_ptr, location::PUBLIC, AssembleMode::All);
    //    SPDLOG_DEBUG("before copy");
    //    kungfu::yijinjing::journal::assemble asb_copy = asb;
    //    SPDLOG_DEBUG("after copy");
    //    auto headers = asb.read_headers(Location{});
    //    for (const auto &head : headers) {
    //      SPDLOG_INFO("head: {}", head.to_string());
    //    }
    //    auto headers_copy = asb_copy.read_headers(Location{});
    //    for (const auto &head : headers_copy) {
    //      SPDLOG_INFO("head_copy: {}", head.to_string());
    //    }
    //
    //    //    context->req_deregister();
    //    kungfu::yijinjing::journal::assemble asb2(l_ptr, location::PUBLIC, AssembleMode::All);
    //    auto locations = asb2.read_bytes<Location>();
    //    SPDLOG_INFO("locations.length: {}", locations.size());
    //    for (const auto &loc : locations) {
    //      SPDLOG_INFO("locaton byte: {}", std::string(loc.second.begin(), loc.second.end()));
    //    }
    //    kungfu::yijinjing::journal::assemble asb3(l_ptr, location::PUBLIC, AssembleMode::All);
    //    auto l3 = asb3.read_all<Location>();
    //    SPDLOG_INFO("locations.length: {}", l3.size());
    //    for (const auto &loc : l3) {
    //      SPDLOG_INFO("l3 : {}", loc.to_string());
    //    }
  }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location) override {
    //    SPDLOG_INFO("on quote: {} i {} location->uid {}", quote.last_price, i, location->location_uid);
    SPDLOG_INFO("Quote: {}", quote.to_string());

    static bool test = true;
    static bool test2 = true;
    //    auto l_ptr = location::make_shared(mode::LIVE, category::MD, "sim", "sim", std::make_shared<locator>());
    auto l_ptr = location::make_shared(mode::LIVE, category::MD, "xtp", "xtp", std::make_shared<locator>());
    if (test) {
      kungfu::yijinjing::journal::reader reader_copy1(*context->get_reader());
      kungfu::yijinjing::journal::reader reader_copy2(*context->get_reader());

      std::this_thread::sleep_for(std::chrono::seconds(10));
      auto j1 = context->get_reader()->get_journal_ref(l_ptr, 0);
      SPDLOG_INFO("j1: {}", j1.get_source());
      auto j2 = context->get_reader()->get_journal_ref(l_ptr, 0);
      int64_t now = kungfu::yijinjing::time::now_in_nano();

      while (j1.current_frame()->has_data() and j1.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("j1 source: {}, dest: {}, msg_type: {}, gen_time: {}", j1.current_frame()->source(),
                     j1.current_frame()->dest(), j1.current_frame()->msg_type(), j1.current_frame()->gen_time());
        //        if (j1.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = j1.current_frame()->data<Quote>();
        //          SPDLOG_WARN("j1 Quote: {}", q.to_string());
        //        }
        j1.next();
      }

      while (j2.current_frame()->has_data() and j2.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("j2 source: {}, dest: {}, msg_type: {}, gen_time: {}", j2.current_frame()->source(),
                     j2.current_frame()->dest(), j2.current_frame()->msg_type(), j2.current_frame()->gen_time());
        //        if (j2.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = j2.current_frame()->data<Quote>();
        //          SPDLOG_WARN("j2 Quote: {}", q.to_string());
        //        }
        j2.next();
      }

      while (reader_copy1.data_available() and reader_copy1.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("1 source: {}, dest: {}, msg_type: {}", reader_copy1.current_frame()->source(),
                     reader_copy1.current_frame()->dest(), reader_copy1.current_frame()->msg_type());
        //        if (reader_copy1.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = reader_copy1.current_frame()->data<Quote>();
        //          SPDLOG_WARN("reader_copy1 Quote: {}", q.to_string());
        //        }
        reader_copy1.next();
      }

      reader_copy2.keep_only(l_ptr->location_uid, 0);
      while (reader_copy2.data_available() and reader_copy2.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("2 source: {}, dest: {}, msg_type: {}", reader_copy2.current_frame()->source(),
                     reader_copy2.current_frame()->dest(), reader_copy2.current_frame()->msg_type());
        //        if (reader_copy2.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = reader_copy2.current_frame()->data<Quote>();
        //          SPDLOG_WARN("reader_copy2 Quote: {}", q.to_string());
        //        }
        reader_copy2.next();
      }
    }

    if (test2 and not test) {
      std::this_thread::sleep_for(std::chrono::seconds(10));
      int64_t now = kungfu::yijinjing::time::now_in_nano();
      auto &j1 = context->get_reader()->get_journal_ref(l_ptr, 0);
      auto &j2 = context->get_reader()->get_journal_ref(l_ptr, 0);
      SPDLOG_DEBUG("ref j1 source: {}, dest: {}, msg_type: {}, gen_time: {}", j1.current_frame()->source(),
                   j1.current_frame()->dest(), j1.current_frame()->msg_type(), j1.current_frame()->gen_time());
      while (j1.current_frame()->has_data() and j1.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("ref j1 source: {}, dest: {}, msg_type: {}, gen_time: {}", j1.current_frame()->source(),
                     j1.current_frame()->dest(), j1.current_frame()->msg_type(), j1.current_frame()->gen_time());
        //        if (j1.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = j1.current_frame()->data<Quote>();
        //          SPDLOG_WARN("ref j1 Quote: {}", q.to_string());
        //        }
        j1.next();
      }

      std::this_thread::sleep_for(std::chrono::seconds(10));
      now = kungfu::yijinjing::time::now_in_nano();
      SPDLOG_DEBUG("ref j2 source: {}, dest: {}, msg_type: {}, gen_time: {}", j2.current_frame()->source(),
                   j2.current_frame()->dest(), j2.current_frame()->msg_type(), j2.current_frame()->gen_time());
      while (j2.current_frame()->has_data() and j1.current_frame()->gen_time() < now) {
        SPDLOG_DEBUG("ref j2 source: {}, dest: {}, msg_type: {}, gen_time: {}", j2.current_frame()->source(),
                     j2.current_frame()->dest(), j2.current_frame()->msg_type(), j2.current_frame()->gen_time());
        //        if (j2.current_frame()->msg_type() == Quote::tag) {
        //          const auto &q = j2.current_frame()->data<Quote>();
        //          SPDLOG_WARN("ref j2 Quote: {}", q.to_string());
        //        }
        j2.next();
      }
      SPDLOG_ERROR("over");
      test2 = false;
    }
    test = false;
  }

  void on_synthetic_data(Context_ptr & context, const SyntheticData &synthetic_data, const location_ptr &location)
      override {
    SPDLOG_INFO("on_synthetic_data: {} ", synthetic_data.to_string());
  }

  void on_broker_state_change(Context_ptr & context, const BrokerStateUpdate &broker_state_update,
                              const location_ptr &location) override {
    SPDLOG_INFO("on broker state changed: {}", broker_state_update.to_string());
  };

  void on_operator_state_change(Context_ptr & context, const OperatorStateUpdate &operator_state_update,
                                const location_ptr &location) override {
    SPDLOG_INFO("on operator state changed: {}", operator_state_update.to_string());
  };

  void on_tree(Context_ptr & context, const Tree &tree, const location_ptr &location) override {
    SPDLOG_INFO("on tree: {}", tree.to_string());
  }

  void on_custom_data(Context_ptr & context, uint32_t msg_type, const std::vector<uint8_t> &data, uint32_t length,
                      const kungfu::yijinjing::data::location_ptr &location) override {
    SPDLOG_WARN("on_custom_data msg_type: {}", msg_type);
    SPDLOG_WARN("on_custom_data data: {}", reinterpret_cast<const char *>(data.data()));
    SPDLOG_WARN("on_custom_data length: {}", length);
  }
};
