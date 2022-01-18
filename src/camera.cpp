#include "camera.hpp"

#include <iostream>

Camera::Camera(const glm::vec3 &position,
               const glm::vec3 &world_up,
               const float      yaw,
               const float      pitch)
    : position_(position),
      front_(glm::vec3(0.0f, 0.0f, -1.0f)),
      world_up_(world_up),
      yaw_(yaw),
      pitch_(pitch),
      movement_speed_(SPEED),
      mouse_sensitivity_(SENSITIVITY),
      zoom_(ZOOM)
{
  update_camera_vectors();
}

glm::mat4 Camera::view_matrix() const
{
  return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::process_movement(const CameraMovement direction,
                              const float          delta_time)
{
  const auto velocity = movement_speed_ * delta_time;

  const auto front = free_fly_ ? front_ : front_movement_;

  if (direction == CameraMovement::Forward)
  {
    position_ += front * velocity;
  }
  if (direction == CameraMovement::Backward)
  {
    position_ -= front * velocity;
  }
  if (direction == CameraMovement::Left)
  {
    position_ -= right_ * velocity;
  }
  if (direction == CameraMovement::Right)
  {
    position_ += right_ * velocity;
  }
}

void Camera::process_rotation(float      xoffset,
                              float      yoffset,
                              const bool constrain_pitch)
{
  xoffset *= mouse_sensitivity_;
  yoffset *= mouse_sensitivity_;

  yaw_ += xoffset;
  pitch_ += yoffset;

  // Make sure that when pitch is out of bounds, screen doesn't get flipped
  if (constrain_pitch)
  {
    if (pitch_ > 89.0f)
    {
      pitch_ = 89.0f;
    }
    if (pitch_ < -89.0f)
    {
      pitch_ = -89.0f;
    }
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  update_camera_vectors();
}

void Camera::process_scroll(float yoffset)
{
  zoom_ -= yoffset;

  if (zoom_ < 1.0f)
  {
    zoom_ = 1.0f;
  }
  if (zoom_ > 45.0f)
  {
    zoom_ = 45.0f;
  }
}

void Camera::update_camera_vectors()
{
  // Calculate the new Front vector
  glm::vec3 f;
  f.x    = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  f.y    = sin(glm::radians(pitch_));
  f.z    = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front_ = glm::normalize(f);

  f.x             = cos(glm::radians(yaw_)) * cos(glm::radians(0.0f));
  f.y             = sin(glm::radians(0.0f));
  f.z             = sin(glm::radians(yaw_)) * cos(glm::radians(0.0f));
  front_movement_ = glm::normalize(f);

  // Also re-calculate the Right and Up vector
  // normalize the vectors, because their length
  // gets closer to 0 the more you look up or down
  // which results in slower movement.
  right_ = glm::normalize(glm::cross(front_, world_up_));

  up_ = glm::normalize(glm::cross(right_, front_));
}

void Camera::set_position(const glm::vec3 &value) { position_ = value; }

void Camera::set_free_fly(bool value) { free_fly_ = value; }

glm::vec3 Camera::front_movement() const { return front_movement_; }

glm::vec3 Camera::right() const { return right_; }

glm::vec3 Camera::front() const { return front_; }

void Camera::set_movement_speed(float value) { movement_speed_ = value; }

float Camera::pitch() const { return pitch_; }

void Camera::set_pitch(float value)
{
  pitch_ = value;
  update_camera_vectors();
}
