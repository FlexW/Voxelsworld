add_library(gl STATIC)
set_warnings_as_errors(gl)
target_compile_features(gl PUBLIC cxx_std_17)

target_link_libraries(gl PUBLIC glad glm)

target_sources(gl PRIVATE
  gl_shader.cpp
  gl_index_buffer.cpp
  gl_vertex_buffer.cpp
  gl_texture.cpp
  gl_texture_array.cpp
  )
