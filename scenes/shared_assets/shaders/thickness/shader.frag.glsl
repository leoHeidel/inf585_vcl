#version 330 core

// Shader use: draw particles' reverse depth buffer to attached framebuffer (used for thickness)

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

float near = 3.0;
float far  = 10.0;

vec3 light = vec3(0.0, -2.0, -2.0);
vec3 n; // normal
vec4 pixelPos; // fragment position in world space
vec4 viewSpacePos; // fragment position in view space
float diffuse;
float viewDepth;
float depth; // output depth

float LinearizeDepth(float depth)
{
    return - (depth + near)/(far - near);
}

void main()
{
    n.xy = 2.0*fragment.texture_uv.xy - 1.0;
    if(length(n.xy) < 1.0){
      n.z = sqrt(1.0 - dot(n.xy, n.xy));
      pixelPos = vec4(fragment.position.xyz - n*radius, 1.0); //note the minus n*radius compared to depth shader
      viewSpacePos = view * pixelPos;
      diffuse = max(0.0, dot(normalize(light - fragment.position.xyz),rotation*n));
      viewDepth = viewSpacePos.z / viewSpacePos.w;
      depth = LinearizeDepth(viewDepth);
      FragColor = vec4(vec3(depth), 1.0);
    }else{
      discard;
    }
}
