#include <kungfu/wingchun/factor/crosssection.h>

namespace kungfu::wingchun::factor {

CrossSection CrossSection::loads(const std::string &serialized_cross_section) {
  CrossSection cross_section;
  cross_section.from_string(serialized_cross_section);
  return cross_section;
}

std::string CrossSection::dumps(const CrossSection &cross_section) {
  return cross_section.to_string();
}

std::string CrossSection::to_string() const {
  nlohmann::json j;
  j["cross_sectional_factor"] = cross_sectional_factor_;
  j["cross_sectional_price"] = cross_sectional_price_;
  j["gen_time"] = gen_time_;
  return j.dump();
}

void CrossSection::from_string(const std::string &serialized_cross_section) {
  nlohmann::json j = nlohmann::json::parse(serialized_cross_section);
  cross_sectional_factor_ = j["cross_sectional_factor"].get<std::unordered_map<std::string, float>>();
  cross_sectional_price_ = j["cross_sectional_price"].get<std::unordered_map<std::string, float>>();
  gen_time_ = j["gen_time"].get<double>();
}

} // namespace kungfu::wingchun::factor