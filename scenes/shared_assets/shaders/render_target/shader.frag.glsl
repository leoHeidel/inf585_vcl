#version 330 core

in struct fragment_data
{
    vec4 position;
    vec4 normal;
    vec4 color;
    vec2 texture_uv;
} fragment;

uniform sampler2D depth_tex_sampler;
uniform sampler2D rev_depth_tex_sampler;

out vec4 FragColor;

uniform vec3 camera_position;
uniform vec3 color     = vec3(1.0, 1.0, 1.0);
uniform float color_alpha = 1.0;

vec3 light = vec3(camera_position.x, camera_position.y, camera_position.z);
float h = 0.001;

void main()
{
    vec3 depth = texture(depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 rev_depth = texture(rev_depth_tex_sampler, fragment.texture_uv).xyz;

    FragColor = vec4(depth, 1.0);
}
