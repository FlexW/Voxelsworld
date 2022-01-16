#include "aabb.hpp"

Aabb::Aabb(const glm::vec3 &min_extent, const glm::vec3 &max_extent)
    : min_extent_(min_extent),
      max_extent_(max_extent)
{
}

std::optional<glm::vec3> Aabb::intersect(const Ray &ray)
{
  float tmin = 0.0f;    // set to -FLT_MAX to get first hit on line
  float tmax = FLT_MAX; // set to max distance ray can travel (for segment)

  const auto p = ray.origin();
  const auto d = ray.direction();

  const auto EPSILON = 0.000001f;

  // For all three slabs
  for (int i = 0; i < 3; i++)
  {
    if (glm::abs(d[i]) < EPSILON)
    {
      // Ray is parallel to slab. No hit if origin not within slab
      if (p[i] < min_extent_[i] || p[i] > max_extent_[i])
        return {};
    }
    else
    {
      // Compute intersection t value of ray with near and far plane of slab
      const float ood = 1.0f / d[i];
      float       t1  = (min_extent_[i] - p[i]) * ood;
      float       t2  = (max_extent_[i] - p[i]) * ood;
      // Make t1 be intersection with near plane, t2 with far plane
      if (t1 > t2)
      {
        std::swap(t1, t2);
      }
      // Compute the intersection of slab intersection intervals
      if (t1 > tmin)
      {
        tmin = t1;
      }
      if (t2 > tmax)
      {
        tmax = t2;
      }
      // Exit with no collision as soon as slab intersection becomes empty
      if (tmin > tmax)
      {
        return {};
      }
    }
  }
  // Ray intersects all 3 slabs. Return point (q) and intersection t value
  // (tmin)
  const auto q = p + d * tmin;

  return q;
}
