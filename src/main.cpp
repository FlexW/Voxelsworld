// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "debug_draw.hpp"
#include "gl/gl_index_buffer.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_vertex_buffer.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtx/string_cast.hpp"
#include "player.hpp"
#include "world.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
constexpr auto opengl_version_major = 4;
constexpr auto opengl_version_minor = 6;

constexpr auto opengl_debug = false;

auto window_width  = 1280;
auto window_height = 720;

auto           is_draw_wireframe = false;
auto           is_cull_face      = true;
constexpr auto is_cursor_enabled = false;

constexpr auto camera_near = 0.1f;
constexpr auto camera_far  = 800.0f;

constexpr auto window_title = "OpenGL App";

void glfw_error_callback(int error_code, const char *description)
{
  std::cerr << "GLFW Error [" << error_code << "]: " << description
            << std::endl;
}

void APIENTRY gl_debug_callback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei /*length*/,
                                const GLchar *msg,
                                const void * /*param*/)
{
  std::string source_str;
  switch (source)
  {
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    source_str = "WindowSys";
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    source_str = "App";
    break;
  case GL_DEBUG_SOURCE_API:
    source_str = "OpenGL";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    source_str = "ShaderCompiler";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    source_str = "3rdParty";
    break;
  case GL_DEBUG_SOURCE_OTHER:
    source_str = "Other";
    break;
  default:
    source_str = "Unknown";
  }

  std::string type_str;
  switch (type)
  {
  case GL_DEBUG_TYPE_ERROR:
    type_str = "Error";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    type_str = "Deprecated";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    type_str = "Undefined";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    type_str = "Portability";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    type_str = "Performance";
    break;
  case GL_DEBUG_TYPE_MARKER:
    type_str = "Marker";
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    type_str = "PushGrp";
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    type_str = "PopGrp";
    break;
  case GL_DEBUG_TYPE_OTHER:
    type_str = "Other";
    break;
  default:
    type_str = "Unknown";
  }

  std::string sev_str;
  switch (severity)
  {
  case GL_DEBUG_SEVERITY_HIGH:
    sev_str = "HIGH";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    sev_str = "MED";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    sev_str = "LOW";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    sev_str = "NOTIFY";
    break;
  default:
    sev_str = "UNK";
  }

  std::cerr << "OpenGL" << source_str << ":" << type_str << "[" << sev_str
            << "]"
            << "(" << id << "): " << msg << std::endl;
}

void gl_dump_info()
{
  const GLubyte *renderer     = glGetString(GL_RENDERER);
  const GLubyte *vendor       = glGetString(GL_VENDOR);
  const GLubyte *version      = glGetString(GL_VERSION);
  const GLubyte *glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

  GLint major, minor, samples, sampleBuffers;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  glGetIntegerv(GL_SAMPLES, &samples);
  glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);

  std::printf(
      "-------------------------------------------------------------\n");
  std::printf("GL Vendor    : %s\n", vendor);
  std::printf("GL Renderer  : %s\n", renderer);
  std::printf("GL Version   : %s\n", version);
  std::printf("GL Version   : %d.%d\n", major, minor);
  std::printf("GLSL Version : %s\n", glsl_version);
  std::printf("MSAA samples : %d\n", samples);
  std::printf("MSAA buffers : %d\n", sampleBuffers);
  std::printf(
      "-------------------------------------------------------------\n");
}

Player player;

void window_framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  window_width  = width;
  window_height = height;
}

void window_close_callback(GLFWwindow *window)
{
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void key_callback(GLFWwindow *window,
                  int         key,
                  int         scancode,
                  int         action,
                  int         mods)
{
  // Toggle wireframe
  if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
  {
    if (is_draw_wireframe)
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    is_draw_wireframe = !is_draw_wireframe;
  }

  // Toggle back face culling
  if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
  {
    if (is_cull_face)
    {
      glDisable(GL_CULL_FACE);
    }
    else
    {
      glEnable(GL_CULL_FACE);
    }
    is_cull_face = !is_cull_face;
  }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
  player.on_mouse_button(button, action, mods);
}

void mouse_movement_callback(GLFWwindow *window, double x, double y)
{
  player.on_mouse_movement(x, y);
}
} // namespace

int main()
{
  if (!glfwInit())
  {
    return EXIT_FAILURE;
  }

  glfwSetErrorCallback(glfw_error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_version_major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_version_minor);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  if (opengl_debug)
  {
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  }

  const auto window = glfwCreateWindow(window_width,
                                       window_height,
                                       window_title,
                                       nullptr,
                                       nullptr);
  if (!window)
  {
    std::cerr << "Could not create GLFW window" << std::endl;
    return EXIT_FAILURE;
  }

  // Set here instead of nullptr data you want to receive in the callback
  glfwSetWindowUserPointer(window, nullptr);
  glfwSetMonitorUserPointer(glfwGetPrimaryMonitor(), nullptr);

  glfwMakeContextCurrent(window);

  // Enable vsync
  glfwSwapInterval(0);

  glfwSetFramebufferSizeCallback(window, window_framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_movement_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowCloseCallback(window, window_close_callback);

  if (is_cursor_enabled)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
  else
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  if (!gladLoadGL())
  {
    std::cerr << "GLAD could not load OpenGL" << std::endl;
  }

  glfwWindowHint(GLFW_SAMPLES, 8);

  if (opengl_debug)
  {
    glDebugMessageCallback(gl_debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE,
                          GL_DONT_CARE,
                          GL_DONT_CARE,
                          0,
                          nullptr,
                          GL_TRUE);
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
                         GL_DEBUG_TYPE_MARKER,
                         0,
                         GL_DEBUG_SEVERITY_NOTIFICATION,
                         -1,
                         "Start debugging");
  }

  gl_dump_info();

  if (is_cull_face)
  {
    glEnable(GL_CULL_FACE);
  }
  else
  {
    glDisable(GL_CULL_FACE);
  }
  glEnable(GL_DEPTH_TEST);
  if (is_draw_wireframe)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  try
  {
    auto world = std::make_unique<World>();
    world->init();

    auto debug_draw = std::make_unique<DebugDraw>();

    GlShader shader;
    shader.init("shaders/blinn_phong.vert", "shaders/blinn_phong.frag");

    auto last_time    = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();

    while (glfwWindowShouldClose(window) == GLFW_FALSE)
    {
      glfwPollEvents();

      current_time = std::chrono::high_resolution_clock::now();
      const auto elapsed_time_ms =
          std::chrono::duration<double, std::milli>(current_time - last_time)
              .count() /
          1000.0;

      // Process movement
      player.update(window, *world, *debug_draw, elapsed_time_ms);

      if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      {
        break;
      }

      world->set_player_position(player.position());
      // std::cout << "Position: " << glm::to_string(camera.position())
      //           << std::endl;

      const auto projection_matrix =
          glm::perspective(glm::radians(player.zoom()),
                           static_cast<float>(window_width) / window_height,
                           camera_near,
                           camera_far);

      glViewport(0, 0, window_width, window_height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

      {
        // Draw world
        shader.bind();
        shader.set_uniform("model_matrix", glm::mat4(1.0f));
        shader.set_uniform("view_matrix", player.view_matrix());
        shader.set_uniform("projection_matrix", projection_matrix);

        // Lights
        shader.set_uniform("point_light_count", 0);
        shader.set_uniform("spot_light_count", 0);
        shader.set_uniform("directional_light_enabled", true);
        shader.set_uniform("directional_light.direction",
                           glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f)));
        shader.set_uniform("directional_light.ambient_color", glm::vec3(0.6f));
        shader.set_uniform("directional_light.diffuse_color", glm::vec3(0.9f));
        shader.set_uniform("directional_light.specular_color", glm::vec3(1.0f));

        world->draw(shader);
        shader.unbind();

        // Draw coordinate system
        debug_draw->draw_line(glm::vec3(0.0f),
                              glm::vec3(10.0f, 0.0f, 0.0f),
                              glm::vec3(1.0f, 0.0f, 0.0f));
        debug_draw->draw_line(glm::vec3(0.0f),
                              glm::vec3(0.0f, 10.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));
        debug_draw->draw_line(glm::vec3(0.0f),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, 1.0f));

        // Debug draw
        debug_draw->submit(player.view_matrix(), projection_matrix);
      }

      glfwSwapBuffers(window);
    }
  }
  catch (const std::runtime_error &error)
  {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
