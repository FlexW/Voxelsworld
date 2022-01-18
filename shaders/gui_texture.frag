#version 460 core

in VS_OUT
{
    vec2 tex_coords;
}
fs_in;

uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(texture(tex, fs_in.tex_coords).rgb, 1.0f);
    // out_color = vec4(fs_in.tex_coords.x, fs_in.tex_coords.y, 0.0f, 1.0f);
}
