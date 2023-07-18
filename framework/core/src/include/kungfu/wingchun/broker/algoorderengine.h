#ifndef WINGCHUN_ALGO_ORDER_ENGINE_H
#define WINGCHUN_ALGO_ORDER_ENGINE_H

#include <kungfu/common.h>
#include <kungfu/wingchun/common.h>
#include <kungfu/wingchun/broker/trader.h>

namespace kungfu::wingchun::broker {
  namespace algoorder {

    class AlgoOrderEngine {
        public:
            explicit AlgoOrderEngine(broker::TraderVendor &vendor);
            virtual ~AlgoOrderEngine() = default;

            void on_start(const rx::connectable_observable<event_ptr> &events);

          private:
            broker::TraderVendor& vendor_;

            BrokerService_ptr get_service();

            void update_algo_order(uint32_t source, const longfist::types::AlgoOrderInput &algo_order_input);
            
            void update_algo_order(int64_t trigger_time, const longfist::types::Order &order);
    };
  } //namespace algoorder

} // namespace kungfu::wingchun::broker

#endif // WINGCHUN_ALGO_ORDER_ENGINE_H