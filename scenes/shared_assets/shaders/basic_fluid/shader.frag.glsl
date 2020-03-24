#version 330 core

// Shader use: draw particles' depth buffer to attached framebuffer

in struct fragment_data
{
    vec4 position;
    vec4 normal;
    vec4 color;
    vec2 texture_uv;
} fragment;

out vec4 FragColor;

uniform vec3 camera_position;
uniform float radius;

// model transformation
uniform mat3 rotation = mat3(1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0); // user defined rotation

// view transform
uniform mat4 view;
// perspective matrix
uniform mat4 perspective;

vec3 light = rotation*vec3(0.0, 5.0, 2.0);
vec3 n; // normal
float diffuse;

void main()
{
    n.xy = 2.0*fragment.texture_uv.xy - 1.0;
    if(length(n.xy) < 1.0){
      n.z = sqrt(1.0 - dot(n.xy, n.xy));
      diffuse = max(0.0, dot(normalize(light - fragment.position.xyz),rotation*n));
      FragColor = vec4(vec3(diffuse * vec3(0.2, 0.7, 1.0)), 1.0);
    }else{
      discard;
    }
}
