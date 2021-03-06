#include "application.hpp"
#include "event.hpp"
#include "gl/gl_shader.hpp"
#include "log/log.hpp"
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
  LOG_ERROR() << "GLFW Error code: " << error_code
              << " Description: " << description;
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

  switch (severity)
  {
  case GL_DEBUG_SEVERITY_HIGH:
    LOG_ERROR() << source_str << " Type: " << type_str << " Id: " << id
                << " Message: " << msg;
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    LOG_WARN() << source_str << " Type: " << type_str << " Id: " << id
               << " Message: " << msg;
    break;
  case GL_DEBUG_SEVERITY_LOW:
    LOG_WARN() << source_str << " Type: " << type_str << " Id: " << id
               << " Message: " << msg;
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    LOG_DEBUG() << source_str << " Type: " << type_str << " Id: " << id
                << " Message: " << msg;
    break;
  default:
    LOG_WARN() << source_str << " Type: " << type_str << " Id: " << id
               << " Message: " << msg;
  }
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
  const auto renderer     = glGetString(GL_RENDERER);
  const auto vendor       = glGetString(GL_VENDOR);
  const auto version      = glGetString(GL_VERSION);
  const auto glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

  GLint major, minor, samples, sampleBuffers;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  glGetIntegerv(GL_SAMPLES, &samples);
  glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);

  GLint extensions_count = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

  LOG_INFO() << "GL Vendor: " << vendor;
  LOG_INFO() << "GL Renderer: " << renderer;
  LOG_INFO() << "GL Version: " << version;
  LOG_INFO() << "GLSL Version: " << glsl_version;
  LOG_INFO() << "MSAA samples: " << samples;
  LOG_INFO() << "MSAA buffers: " << sampleBuffers;

  std::string extensions;
  for (GLint i = 0; i < extensions_count; ++i)
  {
    const auto extension = glGetStringi(GL_EXTENSIONS, i);
    if (i == 0)
    {
      extensions += reinterpret_cast<const char *>(extension);
    }
    else
    {
      extensions +=
          std::string(", ") + reinterpret_cast<const char *>(extension);
    }
  }
  LOG_DEBUG() << "GL Extensions: " << extensions;
}
} // namespace

EventId WindowResizeEvent::id = 0xd1470b85;

WindowResizeEvent::WindowResizeEvent(int width, int height)
    : Event(id),
      width_{width},
      height_{height}
{
}

int WindowResizeEvent::width() const { return width_; }

int WindowResizeEvent::height() const { return height_; }

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
    LOG_ERROR() << "Unhandled exception: " << error.what();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

Config Application::config() const { return config_; }

void Application::init()
{
  config_.load_config("data/voxelworld.ini");

  // Load debug level
  const auto debug_level_str =
      config_.config_value_string("General", "debug_level", "debug");
  if (debug_level_str == "debug")
  {
    Log::set_reporting_level(LogLevel::Debug);
  }
  else if (debug_level_str == "info")
  {
    Log::set_reporting_level(LogLevel::Info);
  }
  else if (debug_level_str == "warn")
  {
    Log::set_reporting_level(LogLevel::Warning);
  }
  else if (debug_level_str == "error")
  {
    Log::set_reporting_level(LogLevel::Error);
  }
  else
  {
    LOG_WARN() << "Unknown debug level: " << debug_level_str;
  }

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
    LOG_ERROR() << "GLAD could not load OpenGL";
    return;
  }

  const auto samples = config_.config_value_int("OpenGL", "samples", 8);
  glEnable(GL_SAMPLES);
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
  glEnable(GL_CLIP_DISTANCE0);

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

  // Setup gui
  gui_ = std::make_unique<Gui>();

  // Create world
  world_ = std::make_unique<World>();
  world_->init();

  // Create player
  player_ = std::make_unique<Player>();

  // Load sky color
  const auto sky_color_r =
      config_.config_value_float("World", "sky_color_r", 0.0f);
  const auto sky_color_g =
      config_.config_value_float("World", "sky_color_g", 0.0f);
  const auto sky_color_b =
      config_.config_value_float("World", "sky_color_b", 0.0f);
  sky_color_ = glm::vec3{sky_color_r, sky_color_g, sky_color_b};
}

void Application::main_loop()
{
  auto last_time = current_time_millis();

  while (glfwWindowShouldClose(window_) == GLFW_FALSE)
  {
    // Calculate delta time
    delta_time_ = (current_time_millis() - last_time) / 1000.0f;
    last_time   = current_time_millis();

    glfwPollEvents();

    // Dispatch events
    event_manager_.dispatch();

    // Process movement
    player_->update(window_, *world_, *debug_draw_, delta_time_);

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
    glClearColor(sky_color_.r, sky_color_.g, sky_color_.b, 1.0f);

    {
      // Draw world
      world_->draw(player_->camera(), projection_matrix, *debug_draw_);

      // Draw gui
      glDisable(GL_CULL_FACE);
      gui_->draw();
      glEnable(GL_CULL_FACE);

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

void Application::on_window_framebuffer_size_callback(GLFWwindow * /*window*/,
                                                      int width,
                                                      int height)
{
  window_width_  = width;
  window_height_ = height;

  auto event =
      std::make_shared<WindowResizeEvent>(window_width_, window_height_);
  event_manager_.publish(event);
}

void Application::on_window_close_callback(GLFWwindow *window)
{
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Application::on_key_callback(GLFWwindow * /*window*/,
                                  int key,
                                  int /*scancode*/,
                                  int action,
                                  int /*mods*/)
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

void Application::on_mouse_button_callback(GLFWwindow * /*window*/,
                                           int button,
                                           int action,
                                           int mods)
{
  player_->on_mouse_button(button, action, mods);
}

void Application::on_mouse_movement_callback(GLFWwindow * /*window*/,
                                             double x,
                                             double y)
{
  player_->on_mouse_movement(x, y);
}

int Application::window_width() const { return window_width_; }

int Application::window_height() const { return window_height_; }

EventManager *Application::event_manager() { return &event_manager_; }

Gui *Application::gui() { return gui_.get(); }

float Application::delta_time() const { return delta_time_; }
