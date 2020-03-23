#version 330 core

in struct fragment_data
{
    vec4 position;
    vec4 normal;
    vec4 color;
    vec2 texture_uv;
} fragment;

uniform sampler2D texture_sampler;

out vec4 FragColor;

uniform vec3 camera_position;
uniform float diffuse  = 0.8;
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



float LinearizeDepth(float depth)
{
    float z = - depth;
    return (z - near)/(far - near);
}

void main()
{
    vec3 N;
    N.xy = 2.0*fragment.texture_uv.xy - 1.0;
    if(length(N.xy) < 1.0){
      N.z = sqrt(1.0 - dot(N.xy, N.xy));
      vec4 pixelPos = vec4(fragment.position.xyz + N*radius, 1.0);
      vec4 viewSpacePos = view * pixelPos;
      float diffuse = max(0.0, dot(normalize(light - fragment.position.xyz),rotation*N));
      float viewDepth = viewSpacePos.z / viewSpacePos.w;
      float depth = LinearizeDepth(viewDepth);
      FragColor = vec4(vec3(depth), 1.0);
    }else{
      discard;
    }
}