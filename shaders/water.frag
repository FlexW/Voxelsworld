#version 460 core

in VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 tex_coords;
  float fog_factor;
  vec4 clip_space;
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

uniform float specular_power = 200.0f;

uniform DirectionalLight directional_light;

uniform float move_factor;
uniform sampler2D reflection_tex;
uniform sampler2D refraction_tex;
uniform sampler2D dudv_tex;

const float wave_strength = 0.005;

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

void main()
{
    vec2 ndc = ((fs_in.clip_space.xy / fs_in.clip_space.w) / 2.0) + 0.5;
    vec2 reflection_tex_coords = vec2(ndc.x, -ndc.y);
    vec2 refraction_tex_coords = ndc;

    vec2 distortion1 = (texture(dudv_tex, 
                               vec2(fs_in.tex_coords.x + move_factor, 
                               fs_in.tex_coords.y)).rg * 2.0 - 1.0) * wave_strength;
    vec2 distortion2 = (texture(dudv_tex, 
                               vec2(-fs_in.tex_coords.x + move_factor, 
                               fs_in.tex_coords.y + move_factor)).rg * 2.0 - 1.0) * wave_strength;
    vec2 total_distortion = distortion1 + distortion2;

    reflection_tex_coords += total_distortion;
    reflection_tex_coords.x = clamp(reflection_tex_coords.x, 0.001, 0.999);
    reflection_tex_coords.y = -clamp(abs(reflection_tex_coords.y), 0.001, 0.999);

    refraction_tex_coords += total_distortion;
    refraction_tex_coords = clamp(refraction_tex_coords, 0.001, 0.999);

    vec4 reflection_color = texture(reflection_tex, reflection_tex_coords);
    vec4 refraction_color = texture(refraction_tex, refraction_tex_coords);

    float refractive_factor = abs(dot(fs_in.normal, vec3(0.0, 1.0, 0.0)) - 1.0);
    refractive_factor = pow(refractive_factor, 0.7);

    vec3 diffuse_color  =  mix(reflection_color,
                               refraction_color, 
                               refractive_factor).rgb;
    vec3 ambient_color  = diffuse_color;
    vec3 specular_color = diffuse_color;

    vec3 color = vec3(0.0f);
    color += blinn_phong_directional_light(ambient_color,
                                         diffuse_color,
                                         specular_color);
    // Add blue for the water
    color = mix(color, vec3(0.0, 0.3, 0.5), 0.2);
 
    // Add fog
    color = fs_in.fog_factor * color + (1.0 - fs_in.fog_factor) * fog_color;

    out_color = vec4(color, 1.0);
}
