#pragma once

class Block
{
public:
  enum class Type
  {
    Grass,
    Air,
  };

  void               set_type(Type type);
  [[nodiscard]] Type type() const;

private:
  Type type_ = Type::Air;
};
