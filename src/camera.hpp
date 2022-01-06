#pragma once

#include "math.hpp"

enum class CameraMovement
{
  Forward,
  Backward,
  Left,
  Right
};

const float YAW         = -90.0f;
const float PITCH       = 0.0f;
const float SPEED       = 0.005f;
const float SENSITIVITY = 0.2f;
const float ZOOM        = 45.0f;

class Camera
{
public:
  Camera(const glm::vec3 &position = glm::vec3(0.0f),
         const glm::vec3 &world_up = glm::vec3(0.0f, 1.0f, 0.0f),
         const float      yaw      = YAW,
         const float      pitch    = PITCH);

  glm::mat4 view_matrix() const;

  void process_movement(const CameraMovement direction, const float delta_time);

  void process_rotation(float      xoffset,
                        float      yoffset,
                        const bool constrain_pitch = true);

  void process_scroll(float yoffset);

  float zoom() const { return zoom_; }

  glm::vec3 position() const { return position_; }

private:
  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 world_up_;

  float yaw_;
  float pitch_;

  float movement_speed_;
  float mouse_sensitivity_;
  float zoom_;

  void update_camera_vectors();
};
