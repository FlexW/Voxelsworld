#pragma once

#include "camera.hpp"
#include "world.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

class DebugDraw;

class Player
{
public:
  Player();

  glm::mat4 view_matrix() const;
  float     zoom() const;
  glm::vec3 position() const;

  void update(GLFWwindow *window,
              World      &world,
              DebugDraw  &debug_draw,
              float       delta_time);

  void on_mouse_movement(double x, double y);
  void on_mouse_button(int button, int action, int mods);

private:
  bool   mouse_first_move_ = true;
  double mouse_last_x_     = 0.0;
  double mouse_last_y_     = 0.0;
  double mouse_offset_x_   = 0.0;
  double mouse_offset_y_   = 0.0;

  Camera camera_;
  bool   free_fly_ = true;

  bool  is_jumping_ = false;
  bool  is_falling_      = false;
  float jump_height_     = 0.0f;
  float max_jump_height_ = 2.0f;

  bool                   do_pick_block_  = false;
  bool                   do_place_block_ = false;
  std::vector<glm::vec3> block_pick_debug_;

  void jump();
};
