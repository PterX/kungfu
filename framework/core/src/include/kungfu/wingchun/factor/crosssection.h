
// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_FACTOR_CROSSSECTION_H
#define WINGCHUN_FACTOR_CROSSSECTION_H

#include <kungfu/longfist/enums.h>
#include <kungfu/longfist/types.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <unordered_map>

namespace kungfu::wingchun::factor {

class CrossSection {
public:
  CrossSection() = default;
  CrossSection(std::unordered_map<std::string, float> cross_sectional_factor,
               std::unordered_map<std::string, float> cross_sectional_price)
      : cross_sectional_factor_(std::move(cross_sectional_factor)),
        cross_sectional_price_(std::move(cross_sectional_price)) {}

  std::string to_string() const;

  void from_string(const std::string &serialized_cross_section);

  static CrossSection loads(const std::string &serialized_cross_section);

  static std::string dumps(const CrossSection &cross_section);

private:
  std::unordered_map<std::string, float> cross_sectional_factor_;
  std::unordered_map<std::string, float> cross_sectional_price_;
  double gen_time_;
};

class MultiCrossSectionalFactor {
public:
  MultiCrossSectionalFactor() = default;

  MultiCrossSectionalFactor(const MultiCrossSectionalFactor &) = delete;

  MultiCrossSectionalFactor &operator=(const MultiCrossSectionalFactor &) = delete;

  virtual ~MultiCrossSectionalFactor() = default;

  void update_price(double price, const std::string &instrument_id, const std::string &exchange_id);

  void update_factor(std::string factor_name, double price, const std::string &instrument_id,
                     const std::string &exchange_id);

  std::map<std::string, CrossSection> generate_cross_sectional_factor(bool clear_price_cache, bool clear_factor_cache);

protected:
  virtual void on_start(const rx::connectable_observable<event_ptr> &events);

  virtual void on_tick(const longfist::types::Tick &tick);

  virtual void on_quote(const longfist::types::Quote &quote);

  virtual void on_entrust(const longfist::types::Entrust &entrust);

  virtual void on_transaction(const longfist::types::Transaction &transaction);

  virtual void on_tree(const longfist::types::Tree &tree);

  virtual void on_depth(const longfist::types::Depth &depth);

private:
  struct TimeStampPrice {
    int64_t time;
    double price;
  };
  std::map<std::string, std::unordered_map<std::string, float>> multi_cross_sectional_factor_cache_;
  std::unordered_map<std::string, TimeStampPrice> price_cache_;
  int64_t now_;
};

} // namespace kungfu::wingchun::factor
#endif // WINGCHUN_FACTOR_CROSSSECTION_H