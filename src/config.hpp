#pragma once

#include <filesystem>
#include <unordered_map>

class Config
{
public:
  void load_config(const std::filesystem::path &file_path);

  float config_value_float(const std::string &section,
                           const std::string &name,
                           float              default_value) const;

  int config_value_int(const std::string &section,
                       const std::string &name,
                       int                default_value) const;

  bool config_value_bool(const std::string &section,
                         const std::string &name,
                         bool               default_value) const;

  std::string config_value_string(const std::string &section,
                                  const std::string &name,
                                  const std::string &default_value) const;

private:
  using KeyValueMap = std::unordered_map<std::string, std::string>;
  std::unordered_map<std::string, KeyValueMap> ini_;
};
