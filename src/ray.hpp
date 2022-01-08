#pragma once

#include "math.hpp"

class Ray
{
public:
  Ray(const glm::vec3 &position, const glm::vec3 &direction);

  void step(float scale);

  glm::vec3 end() const;

  float length() const;

private:
  glm::vec3 ray_start_;
  glm::vec3 ray_end_;
  glm::vec3 ray_direction_;
};
