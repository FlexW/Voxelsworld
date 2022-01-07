#version 460 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

out VS_OUT
{
  vec3 color;
} vs_out;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

void main()
{
  vec4 position = vec4(in_position, 1.0);

  vec4 P = view_matrix * position;

  vs_out.color = in_color;

  gl_Position = projection_matrix * P;
}
