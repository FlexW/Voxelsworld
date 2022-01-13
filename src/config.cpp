#include "config.hpp"
#include "defer.hpp"
#include "string.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

void Config::load_config(const std::filesystem::path &file_path)
{
  std::ifstream ifs(file_path);
  if (!ifs.is_open())
  {
    throw std::runtime_error("Can not open file " + file_path.string());
  }

  // Parse text file
  int         line_index = 0;
  std::string line;
  std::string active_section{};
  while (std::getline(ifs, line))
  {
    defer({ ++line_index; });

    const auto trimmed_line = string_trim(line);
    // Skip comments and blank lines
    if (trimmed_line.empty() || trimmed_line[0] == '#' ||
        trimmed_line[0] == ';')
    {
      continue;
    }

    if (trimmed_line[0] == '[')
    {
      // We have a new section
      // First check that the section string is well formed
      if (trimmed_line[trimmed_line.size() - 1] != ']')
      {
        throw std::runtime_error("Ini file " + file_path.string() +
                                 " section on line " +
                                 std::to_string(line_index) + " is invalid");
      }

      // Remove [] around section name
      auto section_name = trimmed_line;
      section_name.erase(0, 1);
      section_name.erase(section_name.size() - 1, 1);

      // Check that section was not already defined
      const auto section_iter = ini_.find(section_name);
      if (section_iter != ini_.end())
      {
        throw std::runtime_error("Ini file " + file_path.string() +
                                 " section " + section_name +
                                 " already exists");
      }
      // Insert the section and mark it as active
      ini_[section_name] = KeyValueMap{};
      active_section     = section_name;

      // Continue with next line
      continue;
    }

    // Split line on =
    const auto splits = string_split(trimmed_line, "=");
    if (splits.size() != 2)
    {
      throw std::runtime_error("Ini file " + file_path.string() + " line " +
                               std::to_string(line_index) +
                               " is not well formed");
    }
    // Extract name and value
    const auto name  = string_trim(splits[0]);
    const auto value = string_trim(splits[1]);

    // Make sure we have a active section
    if (active_section.empty())
    {
      throw std::runtime_error("Ini file " + file_path.string() + " line " +
                               std::to_string(line_index) +
                               " has no section around it");
    }
    // Insert name and value
    ini_[active_section][name] = value;
  }
}

float Config::config_value_float(const std::string &section,
                                 const std::string &name,
                                 float              default_value) const
{
  const auto section_iter = ini_.find(section);
  if (section_iter != ini_.end())
  {
    const auto key_iter = section_iter->second.find(name);
    if (key_iter != section_iter->second.end())
    {
      return std::atof(key_iter->second.c_str());
    }
  }
  return default_value;
}

int Config::config_value_int(const std::string &section,
                             const std::string &name,
                             int                default_value) const
{
  const auto section_iter = ini_.find(section);
  if (section_iter != ini_.end())
  {
    const auto key_iter = section_iter->second.find(name);
    if (key_iter != section_iter->second.end())
    {
      return std::atoi(key_iter->second.c_str());
    }
  }
  return default_value;
}

bool Config::config_value_bool(const std::string &section,
                               const std::string &name,
                               bool               default_value) const
{
  const auto section_iter = ini_.find(section);
  if (section_iter != ini_.end())
  {
    const auto key_iter = section_iter->second.find(name);
    if (key_iter != section_iter->second.end())
    {
      return std::atoi(key_iter->second.c_str());
    }
  }
  return default_value;
}

std::string Config::config_value_string(const std::string &section,
                                        const std::string &name,
                                        const std::string &default_value) const
{
  const auto section_iter = ini_.find(section);
  if (section_iter != ini_.end())
  {
    const auto key_iter = section_iter->second.find(name);
    if (key_iter != section_iter->second.end())
    {
      return key_iter->second;
    }
  }
  return default_value;
}
