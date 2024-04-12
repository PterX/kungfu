// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_LIVESTREAMDATABATCHER_H
#define WINGCHUN_LIVESTREAMDATABATCHER_H

#include <kungfu/longfist/enums.h>
#include <kungfu/longfist/types.h>
#include <kungfu/wingchun/broker/client.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <kungfu/wingchun/streamdatabatcher/streamdatabatcher.h>

namespace kungfu::wingchun::streamdatabatcher {

class LiveStreamDataBatcher : public StreamDataBatcher {
public:
  void on_entrust(const longfist::types::Entrust &entrust);

  void on_transaction(const longfist::types::Transaction &transaction);

  void on_quote(const longfist::types::Quote &quote);

private:

};

} // namespace kungfu::wingchun::streamdatabatcher
#endif // WINGCHUN_LIVESTREAMDATABATCHER_H