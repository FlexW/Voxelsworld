#version 460 core

in VS_OUT
{
  vec3 color;
}
fs_in;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(fs_in.color, 1.0f);
}
