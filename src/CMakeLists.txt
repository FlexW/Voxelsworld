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
  )

target_link_libraries(app PRIVATE
  gl
  glm
  glad
  glfw
  stb
  )

target_compile_features(app PUBLIC cxx_std_17)
