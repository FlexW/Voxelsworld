#include "player.hpp"
#include "application.hpp"
#include "camera.hpp"
#include "debug_draw.hpp"
#include "ray.hpp"

namespace
{
}

Player::Player() : camera_(glm::vec3(0.0f, 30.0f, 0.0f))
{
  auto       app    = Application::instance();
  const auto config = app->config();

  free_fly_        = config.config_value_bool("Player", "free_fly", free_fly_);
  player_height_   = config.config_value_float("Player", "height", 1.3f);
  gravity_         = config.config_value_float("Player", "gravity", 0.008f);
  const auto speed = config.config_value_float("Player", "speed", 8.0f);

  const auto start_position_x =
      config.config_value_float("Player", "start_positon_x", 0.0f);
  const auto start_position_y =
      config.config_value_float("Player", "start_positon_y", 30.0f);
  const auto start_position_z =
      config.config_value_float("Player", "start_positon_z", 0.0f);

  camera_.set_position(
      glm::vec3{start_position_x, start_position_y, start_position_z});
  camera_.set_free_fly(free_fly_);
  camera_.set_movement_speed(speed);
}

glm::mat4 Player::view_matrix() const { return camera_.view_matrix(); }

float Player::zoom() const { return camera_.zoom(); }

glm::vec3 Player::position() const { return camera_.position(); }

void Player::update(GLFWwindow *window,
                    World      &world,
                    DebugDraw & /*debug_draw*/,
                    float delta_time)
{
  auto position = camera_.position();
  if (!free_fly_)
  {
    if (is_jumping_ && !is_falling_)
    {
      const auto h = gravity_ * 1.0f * delta_time;
      jump_height_ += h;
      position.y += h;
      camera_.set_position(position);

      if (jump_height_ >= max_jump_height_)
      {
        is_falling_ = true;
      }
    }
    else
    {
      const auto is_block = world.is_block(
          glm::ivec3{position.x, position.y - player_height_, position.z});
      if (!is_block)
      {
        position.y -= gravity_ * delta_time;
        camera_.set_position(position);
        if (is_falling_)
        {
          jump_height_ -= position.y;
        }
        if (jump_height_ <= 0.0f)
        {
          jump_height_ = 0.0f;
          is_falling_  = false;
          is_jumping_  = false;
        }
      }
      else
      {
        jump_height_ = 0.0f;
        is_falling_  = false;
        is_jumping_  = false;
      }
    }
  }

  const auto is_move_allowed = [&position, &world](const glm::vec3 &direction)
  {
    for (Ray ray(position, direction); ray.length() < 0.45f; ray.step(0.01f))
    {
      const auto point = ray.end();
      if (world.is_block(point))
      {
        return false;
      }
    }
    return true;
  };

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    if (!free_fly_)
    {
      if (is_move_allowed(camera_.front_movement()))
      {
        camera_.process_movement(CameraMovement::Forward, delta_time);
      }
    }
    else
    {
      camera_.process_movement(CameraMovement::Forward, delta_time);
    }
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    if (!free_fly_)
    {
      if (is_move_allowed(-camera_.front_movement()))
      {
        camera_.process_movement(CameraMovement::Backward, delta_time);
      }
    }
    else
    {
      camera_.process_movement(CameraMovement::Backward, delta_time);
    }
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    if (!free_fly_)
    {
      if (is_move_allowed(camera_.right()))
      {
        camera_.process_movement(CameraMovement::Left, delta_time);
      }
    }
    else
    {
      camera_.process_movement(CameraMovement::Left, delta_time);
    }
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    if (!free_fly_)
    {
      if (is_move_allowed(-camera_.right()))
      {
        camera_.process_movement(CameraMovement::Right, delta_time);
      }
    }
    else
    {
      camera_.process_movement(CameraMovement::Right, delta_time);
    }
  }
  if (!free_fly_ && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    jump();
  }

  // Block picking
  if (do_pick_block_)
  {
    do_pick_block_ = false;
    for (Ray ray(camera_.position(), camera_.front()); ray.length() < 4.0f;
         ray.step(0.5f))
    {
      const auto point = ray.end();
      if (world.remove_block(point))
      {
        break;
      }
    }
  }

  // Block placing
  if (do_place_block_)
  {
    do_place_block_ = false;
    for (Ray ray(camera_.position(), camera_.front()); ray.length() < 10.0f;
         ray.step(0.5f))
    {
      if (world.place_block(ray))
      {
        break;
      }
    }
  }
}

void Player::on_mouse_button(int button, int action, int /*mods*/)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
  {
    do_pick_block_ = true;
  }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
  {
    do_place_block_ = true;
  }
}

void Player::on_mouse_movement(double x, double y)
{
  auto x_offset = 0.0;
  auto y_offset = 0.0;

  if (mouse_first_move_)
  {
    mouse_last_x_ = x;
    mouse_last_y_ = y;

    mouse_first_move_ = false;
  }
  else
  {
    x_offset = x - mouse_last_x_;
    y_offset = mouse_last_y_ - y;

    mouse_offset_x_ = x_offset;
    mouse_offset_y_ = y_offset;

    mouse_last_x_ = x;
    mouse_last_y_ = y;
  }

  camera_.process_rotation(x_offset, y_offset);
}

void Player::jump()
{
  if (is_jumping_)
  {
    return;
  }
  is_jumping_ = true;
}

Camera Player::camera() const { return camera_; }
