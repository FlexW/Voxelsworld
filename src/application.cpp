
#include "application.hpp"
#include "gl/gl_shader.hpp"
#include "player.hpp"
#include "time.hpp"

#include <GL/gl.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
constexpr auto opengl_version_major = 4;
constexpr auto opengl_version_minor = 6;

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

  std::cerr << "OpenGL " << source_str << ":" << type_str << "[" << sev_str
            << "]"
            << "(" << id << "): " << msg << std::endl;
}

void window_framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  assert(app);
  app->on_window_framebuffer_size_callback(window, width, height);
}

void window_close_callback(GLFWwindow *window)
{
  auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  assert(app);
  app->on_window_close_callback(window);
}

void key_callback(GLFWwindow *window,
                  int         key,
                  int         scancode,
                  int         action,
                  int         mods)
{
  auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  assert(app);
  app->on_key_callback(window, key, scancode, action, mods);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
  auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  assert(app);
  app->on_mouse_button_callback(window, button, action, mods);
}

void mouse_movement_callback(GLFWwindow *window, double x, double y)
{
  auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  assert(app);
  app->on_mouse_movement_callback(window, x, y);
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

  GLint extensions_count = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

  std::printf(
      "-------------------------------------------------------------\n");
  std::printf("GL Vendor    : %s\n", vendor);
  std::printf("GL Renderer  : %s\n", renderer);
  std::printf("GL Version   : %s\n", version);
  std::printf("GL Version   : %d.%d\n", major, minor);
  std::printf("GLSL Version : %s\n", glsl_version);
  std::printf("MSAA samples : %d\n", samples);
  std::printf("MSAA buffers : %d\n", sampleBuffers);
  std::printf("GL Extensions:\n");
  for (GLint i = 0; i < extensions_count; ++i)
  {
    const auto extension = glGetStringi(GL_EXTENSIONS, i);
    std::printf("\t%s\n", extension);
  }
  std::printf(
      "-------------------------------------------------------------\n");
}
} // namespace

Application *Application::instance()
{
  static std::unique_ptr<Application> unique{};
  if (!unique)
  {
    unique.reset(new Application);
  }
  return unique.get();
}

int Application::run()
{
  try
  {
    init();
    main_loop();
  }
  catch (const std::runtime_error &error)
  {
    std::cerr << "Unhandled exception: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

Config Application::config() const { return config_; }

void Application::init()
{
  config_.load_config("data/voxelworld.ini");

  if (!glfwInit())
  {
    throw std::runtime_error("Can not init GLFW");
  }

  glfwSetErrorCallback(glfw_error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_version_major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_version_minor);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  opengl_debug_ = config_.config_value_bool("OpenGL", "debug", true);
  if (opengl_debug_)
  {
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  }

  window_width_  = config_.config_value_int("Window", "width", 800);
  window_height_ = config_.config_value_int("Window", "height", 600);
  const auto window_title =
      config_.config_value_string("Window", "title", "Voxelworld");

  window_ = glfwCreateWindow(window_width_,
                             window_height_,
                             window_title.c_str(),
                             nullptr,
                             nullptr);
  if (!window_)
  {
    throw std::runtime_error("Could not create GLFW window");
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetMonitorUserPointer(glfwGetPrimaryMonitor(), nullptr);

  glfwMakeContextCurrent(window_);

  // Enable vsync
  glfwSwapInterval(0);

  glfwSetFramebufferSizeCallback(window_, window_framebuffer_size_callback);
  glfwSetCursorPosCallback(window_, mouse_movement_callback);
  glfwSetMouseButtonCallback(window_, mouse_button_callback);
  glfwSetKeyCallback(window_, key_callback);
  glfwSetWindowCloseCallback(window_, window_close_callback);

  is_cursor_enabled_ =
      config_.config_value_int("Window", "cursor_enabled", false);
  if (is_cursor_enabled_)
  {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
  else
  {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  if (!gladLoadGL())
  {
    std::cerr << "GLAD could not load OpenGL" << std::endl;
  }

  const auto samples = config_.config_value_int("OpenGL", "samples", 8);
  glfwWindowHint(GLFW_SAMPLES, samples);

  if (opengl_debug_)
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

  is_cull_face_ = config_.config_value_bool("OpenGL", "cull_face", true);
  if (is_cull_face_)
  {
    glEnable(GL_CULL_FACE);
  }
  else
  {
    glDisable(GL_CULL_FACE);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  is_draw_wireframe_ = config_.config_value_bool("OpenGL", "wireframe", false);
  if (is_draw_wireframe_)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  is_draw_coordinate_system_ =
      config_.config_value_bool("OpenGL", "coordinate_system", false);
  coordinate_system_length_ =
      config_.config_value_float("OpenGL", "coordinate_system_length", 100.0);

  camera_near_ = config_.config_value_float("Window", "camera_near", 0.1);
  camera_far_  = config_.config_value_float("Window", "camera_far", 800.0);

  debug_draw_ = std::make_unique<DebugDraw>();

  // Create world
  world_ = std::make_unique<World>();
  world_->init();

  // Create player
  player_ = std::make_unique<Player>();
}

void Application::main_loop()
{
  auto  last_time  = current_time_millis();
  float delta_time = 0.0f;

  while (glfwWindowShouldClose(window_) == GLFW_FALSE)
  {
    // Calculate delta time
    delta_time = (current_time_millis() - last_time) / 1000.0f;
    last_time  = current_time_millis();

    glfwPollEvents();

    // Process movement
    player_->update(window_, *world_, *debug_draw_, delta_time);

    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
      break;
    }

    world_->set_player_position(player_->position());

    const auto projection_matrix =
        glm::perspective(glm::radians(player_->zoom()),
                         static_cast<float>(window_width_) / window_height_,
                         camera_near_,
                         camera_far_);

    glViewport(0, 0, window_width_, window_height_);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    {
      // Draw world
      world_->draw(player_->view_matrix(), projection_matrix, *debug_draw_);

      // Draw coordinate system
      if (is_draw_coordinate_system_)
      {
        debug_draw_->draw_line(glm::vec3(0.0f),
                               glm::vec3(coordinate_system_length_, 0.0f, 0.0f),
                               glm::vec3(1.0f, 0.0f, 0.0f));
        debug_draw_->draw_line(glm::vec3(0.0f),
                               glm::vec3(0.0f, coordinate_system_length_, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        debug_draw_->draw_line(glm::vec3(0.0f),
                               glm::vec3(0.0f, 0.0f, coordinate_system_length_),
                               glm::vec3(0.0f, 0.0f, 1.0f));
      }

      // Debug draw
      debug_draw_->submit(player_->view_matrix(), projection_matrix);
    }

    glfwSwapBuffers(window_);
  }
}

void Application::on_window_framebuffer_size_callback(GLFWwindow *window,
                                                      int         width,
                                                      int         height)
{
  window_width_  = width;
  window_height_ = height;
}

void Application::on_window_close_callback(GLFWwindow *window)
{
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Application::on_key_callback(GLFWwindow *window,
                                  int         key,
                                  int         scancode,
                                  int         action,
                                  int         mods)
{
  // Toggle wireframe
  if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
  {
    if (is_draw_wireframe_)
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    is_draw_wireframe_ = !is_draw_wireframe_;
  }

  // Toggle back face culling
  if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
  {
    if (is_cull_face_)
    {
      glDisable(GL_CULL_FACE);
    }
    else
    {
      glEnable(GL_CULL_FACE);
    }
    is_cull_face_ = !is_cull_face_;
  }
}

void Application::on_mouse_button_callback(GLFWwindow *window,
                                           int         button,
                                           int         action,
                                           int         mods)
{
  player_->on_mouse_button(button, action, mods);
}

void Application::on_mouse_movement_callback(GLFWwindow *window,
                                             double      x,
                                             double      y)
{
  player_->on_mouse_movement(x, y);
}
