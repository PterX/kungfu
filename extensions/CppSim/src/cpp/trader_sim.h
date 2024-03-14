#ifndef KUNGFU_XTP_EXT_TRADER_H
#define KUNGFU_XTP_EXT_TRADER_H

#include <kungfu/wingchun/broker/trader.h>

namespace kungfu::wingchun::sim {
enum class MatchMode { Reject, Pending, Cancel, PartialFillAndCancel, PartialFill, Fill, MultipleTransactions };

// map TaskState values to JSON as strings
KF_JSON_SERIALIZE_ENUM(MatchMode, {
                                      {MatchMode::Reject, "Reject"},
                                      {MatchMode::Pending, "Pending"},
                                      {MatchMode::Cancel, "Cancel"},
                                      {MatchMode::PartialFillAndCancel, "PartialFillAndCancel"},
                                      {MatchMode::PartialFill, "PartialFill"},
                                      {MatchMode::Fill, "Fill"},
                                      {MatchMode::MultipleTransactions, "MultipleTransactions"},
                                  })

static const std::map<MatchMode, longfist::enums::OrderStatus> mao_order_status = {
    {MatchMode::Reject, longfist::enums::OrderStatus::Error},
    {MatchMode::Pending, longfist::enums::OrderStatus::Pending},
    {MatchMode::Cancel, longfist::enums::OrderStatus::Cancelled},
    {MatchMode::PartialFillAndCancel, longfist::enums::OrderStatus::PartialFilledNotActive},
    {MatchMode::PartialFill, longfist::enums::OrderStatus::PartialFilledActive},
    {MatchMode::Fill, longfist::enums::OrderStatus::Filled},
    {MatchMode::MultipleTransactions, longfist::enums::OrderStatus::Filled},
};

struct Pos {
  std::string instrument;
  longfist::enums::Direction direction;
  int volume_yesterday{};
  int volume_today{};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Pos, instrument, direction, volume_yesterday, volume_today)
};

struct TDConfiguration {
  std::string account_id;
  MatchMode match_mode;
  bool recover_order_trade;
  double asset;
  std::vector<Pos> position_list;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(TDConfiguration, account_id, match_mode, recover_order_trade, asset, position_list)
};

class TraderSim : public broker::Trader {
public:
  explicit TraderSim(broker::BrokerVendor &vendor);

  ~TraderSim() override = default;

  [[nodiscard]] longfist::enums::AccountType get_account_type() const override {
    return longfist::enums::AccountType::Stock;
  }

  void pre_start() override;

  void on_start() override;

  void on_exit() override;

  bool insert_order(const event_ptr &event) override;

  bool cancel_order(const event_ptr &event) override;

  bool req_position() override;

  //  bool on_custom_event(const event_ptr &event) override;

  bool req_account() override;

  bool req_history_order(const event_ptr &event) override;

  bool req_history_trade(const event_ptr &event) override;

  void on_recover() override;

private:
  TDConfiguration config_{};
  std::string trading_day_{};
  longfist::enums::OrderStatus default_status_{};

  void generate_trade(const longfist::types::Order &order, uint32_t dest_id);

  bool verify_order(longfist::types::Order &order);
};
} // namespace kungfu::wingchun::sim
#endif // KUNGFU_XTP_EXT_TRADER_H
