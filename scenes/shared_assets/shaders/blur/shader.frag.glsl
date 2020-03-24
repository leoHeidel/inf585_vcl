#version 330 core

// Shader use: bilateral gaussian blur on input texture

in struct fragment_data
{
    vec4 position;
    vec4 normal;
    vec4 color;
    vec2 texture_uv;
} fragment;

uniform sampler2D depth_tex_sampler;

out vec4 FragColor;

int samples = 5;
vec2 offset;
float gauss;
float diff; //pixel intensity difference
float s = 0.01; //standard deviation gaussian blur
float r = 0.05; //standard deviation amplitude falloff

void main()
{
    vec3 depth = texture(depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 depth_color = vec3(0.0);
    float sum = 0.0;
    for(int i = 0; i < samples; i++){
      for(int j = 0; j < samples; j++){
        offset = vec2(i/(samples - 1.0) - 0.5, j/(samples - 1.0) - 0.5) * 0.01;
        diff = length(texture(depth_tex_sampler, fragment.texture_uv + offset).xyz - depth);
        gauss = exp(-(dot(offset, offset))/(2*s*s) - (diff * diff)/(2*r*r)) / (2*3.14 * s * r);
        sum += gauss;
        depth_color += texture(depth_tex_sampler, fragment.texture_uv + offset).xyz * gauss;
      }
    }
    depth_color /= sum;

    FragColor = vec4(depth.x == 1.0 ? vec3(1.0) : vec3(depth_color), 1.0);
}
