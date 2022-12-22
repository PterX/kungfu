#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/operator/context.h>
#include <kungfu/wingchun/operator/runtime.h>
#include <kungfu/wingchun/operator/operator.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::op;
using namespace kungfu::yijinjing::data;
int i = 0;
KUNGFU_MAIN_OPERATOR(KungfuOperator101) {
public:
  KungfuOperator101() = default;
  ~KungfuOperator101() = default;

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing operator");
    context->subscribe("sim", {"600000"}, {"SSE"});
  }

  void post_start(Context_ptr & context) override {
    SPDLOG_INFO("operator started");
  }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location) override {
    i++;
    SPDLOG_INFO("on quote: {} i {} location->uid {}", quote.last_price, i, location->location_uid);
    if (i == 5) {
      std::shared_ptr<kungfu::yijinjing::journal::assemble> p_assemble =
          std::make_shared<kungfu::yijinjing::journal::assemble>(std::vector<locator_ptr>{});
      std::shared_ptr<kungfu::yijinjing::journal::frame_reader> r = p_assemble->get_reader(location);
      auto f = r->current_frame();
      SPDLOG_INFO("f source {} dest {} data {}", f->source(), f->dest(), f->data_as_string());
      while (true) {
        auto f = r->next_frame();
        if (!f) {
          SPDLOG_INFO("f null");
          break;
        }
        SPDLOG_INFO("f source {} dest {} data {}", f->source(), f->dest(), f->data_as_string());
      }
    }
  }
};
