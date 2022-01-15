#version 460 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in int in_tex_index;

out VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 tex_coord;
  float fog_factor;
  flat int tex_index;
} vs_out;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform float fog_start = 50.0;
uniform float fog_end = 400.0;

uniform mat4 model_matrix;

void main()
{
  vec4 position = vec4(in_position, 1.0);
  vec3 normal = in_normal;

  mat4 view_model_matrix = view_matrix * model_matrix;

  vec4 P = view_model_matrix * position;
  vec3 N = normalize(transpose(inverse(mat3(view_model_matrix))) * normal);

  vs_out.position = P.xyz;
  vs_out.normal = N;
  vs_out.tex_coord = in_tex_coord;

  vs_out.tex_index = in_tex_index;

  gl_Position = projection_matrix * P;

  // Calculate linear fog
  vs_out.fog_factor = clamp((fog_end - abs(P.z)) / (fog_end - fog_start), 0.0, 1.0);
}
