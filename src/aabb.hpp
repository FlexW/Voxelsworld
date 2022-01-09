#pragma once

#include "ray.hpp"

#include <optional>

class Aabb
{
public:
  Aabb(const glm::vec3 &min_extent, const glm::vec3 &max_extent);

  std::optional<glm::vec3> intersect(const Ray &ray);

private:
  glm::vec3 min_extent_;
  glm::vec3 max_extent_;
};
