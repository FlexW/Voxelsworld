add_executable(app)
set_warnings_as_errors(app)

target_sources(app PRIVATE
  main.cpp
  )

target_link_libraries(app PRIVATE
  glm
  glad
  glfw
  stb
  )

target_compile_features(app PUBLIC cxx_std_17)
