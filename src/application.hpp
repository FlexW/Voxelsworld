#pragma once

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "config.hpp"
#include "debug_draw.hpp"
#include "gl/gl_shader.hpp"
#include "player.hpp"
#include "world.hpp"

#include <memory>

class Application
{
public:
  static Application *instance();

  int run();

  Config config() const;

  void on_window_framebuffer_size_callback(GLFWwindow *window,
                                           int         width,
                                           int         height);
  void on_window_close_callback(GLFWwindow *window);

  void on_key_callback(GLFWwindow *window,
                       int         key,
                       int         scancode,
                       int         action,
                       int         mods);

  void on_mouse_button_callback(GLFWwindow *window,
                                int         button,
                                int         action,
                                int         mods);

  void on_mouse_movement_callback(GLFWwindow *window, double x, double y);

private:
  bool  opengl_debug_;
  bool  is_draw_coordinate_system_;
  float coordinate_system_length_;

  int window_width_;
  int window_height_;

  float camera_near_;
  float camera_far_;

  bool is_cursor_enabled_;
  bool is_cull_face_;
  bool is_draw_wireframe_;

  Config config_;

  std::unique_ptr<World>  world_{};
  std::unique_ptr<Player> player_{};

  std::unique_ptr<DebugDraw> debug_draw_{};

  GLFWwindow *window_{};

  std::unique_ptr<GlShader> world_shader_{};

  Application()                    = default;
  Application(const Application &) = delete;
  void operator=(const Application &) = delete;

  void init();
  void main_loop();
};
