#ifndef CPPSIM_COMMON_H
#define CPPSIM_COMMON_H

#include <kungfu/longfist/types.h>

namespace kungfu::wingchun::sim {
template <typename T> double get_min_volume(const T &data) {
  longfist::enums::InstrumentType t = get_instrument_type(data.exchange_id, data.instrument_id);
  switch (t) {
  case longfist::enums::InstrumentType::Stock:
  case longfist::enums::InstrumentType::TechStock:
    return 100.0;
  default:
    return 1.0;
  }
}
} // namespace kungfu::wingchun::sim

#endif // CPPSIM_COMMON_H
