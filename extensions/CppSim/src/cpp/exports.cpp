#include "marketdata_sim.h"
#include "trader_sim.h"

#include <kungfu/wingchun/extension.h>

KUNGFU_EXTENSION() {
  KUNGFU_DEFINE_MD(kungfu::wingchun::sim::MarketDataSim);
  KUNGFU_DEFINE_TD(kungfu::wingchun::sim::TraderSim);
}
