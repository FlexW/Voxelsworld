#version 460 core

in VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 tex_coord;
  float fog_factor;
  flat int tex_index;
}
fs_in;

layout(location = 0) out vec4 out_color;

struct DirectionalLight
{
  vec3 direction;

  vec3 ambient_color;
  vec3 diffuse_color;
  vec3 specular_color;
};

uniform vec3 fog_color = vec3(0.53f, 0.81f, 0.92f);

uniform sampler2DArray in_diffuse_tex;

uniform float specular_power = 200.0f;

uniform DirectionalLight directional_light;

vec3 blinn_phong_directional_light(vec3 ambient_color,
                                   vec3 diffuse_color,
                                   vec3 specular_color)
{
  vec3 N = fs_in.normal;
  vec3 V = normalize(-fs_in.position);
  vec3 L = normalize(-directional_light.direction);

  // Compute lightning
  vec3  ambient  = ambient_color * directional_light.ambient_color;
  float N_dot_L  = max(dot(N, L), 0.0);
  vec3  diffuse  = N_dot_L * diffuse_color * directional_light.diffuse_color;
  vec3  specular = vec3(0.0);
  if (N_dot_L > 0.0)
  {
    vec3 H   = normalize(L + V);
    specular = specular_color * pow(max(dot(N, H), 0.0), specular_power) *
               directional_light.specular_color;
  }

  return ambient + diffuse + specular;
}

vec3 calc_diffuse_color()
{
    return texture(in_diffuse_tex, vec3(fs_in.tex_coord, float(fs_in.tex_index))).xyz;
}

void main()
{
  vec3 diffuse_color  = calc_diffuse_color();
  vec3 ambient_color  = diffuse_color;
  vec3 specular_color = diffuse_color;

  vec3 color = vec3(0.0f);
  color += blinn_phong_directional_light(ambient_color,
                                         diffuse_color,
                                         specular_color);
  // Add fog
  color = fs_in.fog_factor * color + (1.0 - fs_in.fog_factor) * fog_color;

  out_color = vec4(color, 0.8);
}
