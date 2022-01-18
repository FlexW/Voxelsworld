#pragma once

#include "math.hpp"

class GuiElement
{
public:
  virtual ~GuiElement() = default;

  virtual void draw(float window_width, float window_height) = 0;
};
