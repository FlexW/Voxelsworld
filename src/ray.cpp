#include "ray.hpp"

Ray::Ray(const glm::vec3 &position, const glm::vec3 &direction)
    : ray_start_(position),
      ray_end_(position),
      ray_direction_(direction)
{
}

void Ray::step(float scale) { ray_end_ += ray_direction_ * scale; }

glm::vec3 Ray::end() const { return ray_end_; }

float Ray::length() const { return glm::distance(ray_start_, ray_end_); }
