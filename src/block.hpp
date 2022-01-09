#pragma once

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
    Air,
  };

  void               set_type(Type type);
  [[nodiscard]] Type type() const;

private:
  Type type_ = Type::Air;
};
