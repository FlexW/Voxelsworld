#version 460 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in int in_tex_index;

out VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 tex_coords;
  float fog_factor;
  vec4 clip_space;
} vs_out;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform float fog_start = 50.0;
uniform float fog_end = 400.0;

uniform mat4 model_matrix;

const float tiling = 0.1f;

void main()
{
  vec4 position = vec4(in_position, 1.0);
  vec3 normal = in_normal;

  mat4 view_model_matrix = view_matrix * model_matrix;

  vec4 P = view_model_matrix * position;
  vec3 N = normalize(transpose(inverse(mat3(view_model_matrix))) * normal);

  vs_out.position = P.xyz;
  vs_out.normal = N;


  vs_out.clip_space = projection_matrix * P;
  gl_Position = vs_out.clip_space;

  // Calculate linear fog
  vs_out.fog_factor = clamp((fog_end - abs(P.z)) / (fog_end - fog_start), 0.0, 1.0);

  // Send the texture coords
  vs_out.tex_coords = in_tex_coord * tiling;
}
