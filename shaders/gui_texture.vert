#version 460 core

layout (location = 0) in vec2 in_position;

out VS_OUT
{
  vec2 tex_coords;
} vs_out;

uniform mat4 model_matrix;
uniform mat4 projection_matrix;

void main()
{
    vec2 tex_coords[4];
    tex_coords[0] = vec2(0.0f, 0.0f);
    tex_coords[1] = vec2(0.0f, 1.0f);
    tex_coords[2] = vec2(1.0f, 1.0f);
    tex_coords[3] = vec2(1.0f, 0.0f);

    vs_out.tex_coords = tex_coords[gl_VertexID];
    gl_Position = projection_matrix * model_matrix * vec4(in_position.xy, 0.0f, 1.0f);
}
