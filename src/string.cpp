#include "string.hpp"

#include <string>
#include <vector>

namespace
{
const std::string whitespace = " \n\r\t\f\v";
}

std::string string_trim_left(const std::string &str)
{
  const auto start = str.find_first_not_of(whitespace);
  return (start == std::string::npos) ? "" : str.substr(start);
}

std::string string_trim_right(const std::string &str)
{
  const auto end = str.find_last_not_of(whitespace);
  return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string string_trim(const std::string &str)
{
  return string_trim_right(string_trim_left(str));
}

std::vector<std::string> string_split(const std::string &str,
                                      const std::string &delimiter)
{
  std::string              str_cpy{str};
  std::vector<std::string> tokens;

  std::size_t pos = 0;
  std::string token{};

  while ((pos = str_cpy.find(delimiter)) != std::string::npos)
  {
    token = str_cpy.substr(0, pos);
    tokens.push_back(token);
    str_cpy.erase(0, pos + delimiter.length());
  }
  if (!str_cpy.empty())
  {
    tokens.push_back(str_cpy);
  }

  return tokens;
}
