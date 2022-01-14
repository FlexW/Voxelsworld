#pragma once

#include <cstdint>

class Block
{
public:
  static constexpr int width  = 1;
  static constexpr int height = 1;

  enum class Type
  {
    Grass,
    Dirt,
    Water,
    Oak,
    OakLeaves,
    Air,
  };

  enum class Side
  {
    Top,
    Bottom,
    Left,
    Right,
    Front,
    Back,
  };

  void               set_type(Type type);
  [[nodiscard]] Type type() const;

private:
  Type type_ = Type::Air;
};
