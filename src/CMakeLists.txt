add_subdirectory(log)
add_subdirectory(gl)

add_executable(app)
set_warnings_as_errors(app)

target_sources(app PRIVATE
  main.cpp
  camera.cpp
  world.cpp
  chunk.cpp
  block.cpp
  debug_draw.cpp
  player.cpp
  ray.cpp
  aabb.cpp
  texture_atlas.cpp
  config.cpp
  string.cpp
  application.cpp
  time.cpp
  image.cpp
  event.cpp
  event_manager.cpp
  gui.cpp
  gui_texture.cpp
  )

target_include_directories(app PRIVATE .)

target_link_libraries(app PRIVATE
  gl
  log
  fastdelegate
  glm
  glad
  glfw
  stb
  )

target_compile_features(app PUBLIC cxx_std_17)
