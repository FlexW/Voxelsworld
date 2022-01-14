#pragma once

#include <cstdint>

class Block
{
public:
  static constexpr int width  = 1;
  static constexpr int height = 1;

  enum class Type : std::uint8_t
  {
    Grass = 0,
    Dirt  = 1,
    Water = 2,
    Air   = 3,
  };

  enum class Side : std::uint8_t
  {
    Top    = 0,
    Bottom = 1,
    Left   = 2,
    Right  = 3,
    Front  = 4,
    Back   = 5,
  };

  void               set_type(Type type);
  [[nodiscard]] Type type() const;

private:
  Type type_ = Type::Air;
};
