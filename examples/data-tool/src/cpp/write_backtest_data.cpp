#include <kungfu/wingchun/extension.h>
#include <spdlog/spdlog.h>
#include <kungfu/wingchun/tool/cachetool.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;

KUNGFU_MAIN_TOOL(TestSimDataTool){
  public : TestSimDataTool(category category, std::string source, int64_t start_time, int64_t end_time,
                           locator_ptr locator, bool overwrite = true) :
      CacheTool(category, source, start_time, end_time, locator, overwrite){}
  
  virtual void run() override {
    kungfu::yijinjing::journal::frame_ptr frame;
    for (std::size_t i = 0; i < 100; ++i) {
      Quote quote{};
      quote.instrument_id = "test";
      quote.exchange_id = "SSE";
      quote.bid_price[0] = i;
      quote.ask_price[0] = i + 1;
      quote.data_time = get_begin_time() + i;
      write_at(quote.data_time, quote.data_time, location::PUBLIC, quote);
      frame = current_frame();
      spdlog::info("frame: {}", frame->data<Quote>().to_string());
      assert(frame->msg_type() == Quote::tag);
      assert(frame->data<Quote>().to_string() == quote.to_string());
      next();



      Entrust entrust{};
      entrust.instrument_id = "test";
      entrust.exchange_id = "SSE";
      entrust.price = i;
      entrust.data_time = get_begin_time() + i;
      write_at(entrust.data_time, entrust.data_time, location::PUBLIC, entrust);
      frame = current_frame();
      spdlog::info("frame: {}", frame->data<Entrust>().to_string());
      assert(frame->msg_type() == Entrust::tag);
      assert(frame->data<Entrust>().to_string() == entrust.to_string());
      next();

      Transaction transaction{};
      transaction.instrument_id = "test";
      transaction.exchange_id = "SSE";
      transaction.price = i;
      transaction.data_time = get_begin_time() + i;
      write_at(transaction.data_time, transaction.data_time, location::PUBLIC,  transaction);
      frame = current_frame();
      spdlog::info("frame: {}", frame->data<Transaction>().to_string());
      assert(frame->msg_type() == Transaction::tag);
      assert(frame->data<Transaction>().to_string() == transaction.to_string());
      next();
    }

    // while (data_available()) {
    //   auto frame = current_frame();
    //   switch (frame->msg_type()) {
    //   case Quote::tag:
    //     spdlog::info("frame: {}", frame->data<Quote>().to_string());
    //     break;
    //   case Entrust::tag:
    //     spdlog::info("frame: {}", frame->data<Entrust>().to_string());
    //     break;
    //   case Transaction::tag:
    //     spdlog::info("frame: {}", frame->data<Transaction>().to_string());
    //     break;
    //   default:
    //     break;
    //   }
    //   next();
    // }
  }

};
