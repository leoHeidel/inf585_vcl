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
uniform vec3 color;
uniform float color_alpha = 1.0;
uniform mat3 rotation;

vec3 light = vec3(0.0, -2.0, -2.0); //vec3(camera_position.xy, -camera_position.z); //
float h = 0.01;

float near = 3.0;
float far  = 10.0;
float angle_of_view = 50*3.14159f/180;
float fy = 1/tan(angle_of_view/2);
float fx = fy/1.28f;
float texelSize = 0.002;

vec3 getEyePos(sampler2D depthTex, vec2 texCoord){
  float a = texture(depthTex, texCoord).x;
  //float depth = near * far / (far + a * (near - far)); //getting back the depth in view space
  float depth = - (near + a * (far - near));
  float Wx = (texCoord.x * 2.0 - 1.0) / fx;
  float Wy = (texCoord.y * 2.0 - 1.0) / fy;
  return vec3(Wx, Wy, 1.0) * depth;
}

float LinearizeDepth(float depth)
{
    float z = - depth;
    return (z - near)/(far - near);
}

void main()
{
    vec4 depth = texture(depth_tex_sampler, fragment.texture_uv);
    vec3 thickness = texture(rev_depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 posEye = getEyePos(depth_tex_sampler, fragment.texture_uv);

    vec3 ddx = getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(texelSize, 0)) - posEye;
    vec3 ddx2 = posEye - getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(-texelSize, 0));
    if (abs(ddx.z) > abs(ddx2.z)) {
      ddx = ddx2;
    }

    vec3 ddy = getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(0, texelSize)) - posEye;
    vec3 ddy2 = posEye - getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(0, -texelSize));
    if (abs(ddy.z) > abs(ddy2.z)) {
      ddy = ddy2;
    }

    vec3 n = cross(ddy, ddx);
    vec3 u = normalize(light-fragment.position.xyz);
    n = rotation * normalize(n);
    //FragColor = vec4(n * 0.5 + vec3(0.5), depth.x == 1.0 ? 0.0 : 1.0);

    vec3 r = reflect(u,n);
    vec3 t = normalize(fragment.position.xyz-camera_position);
    float specular_value = 0.5 * pow(clamp(dot(r,t), 0.0, 1.0), 128);
    float R0 = 0.5;
    float fresnel = clamp(R0 + (1.0 - R0)*pow(1-dot(n,t), 5.0), 0.0, 1.0);
    float diffuse = clamp(dot(normalize(light-fragment.position.xyz),n), 0.0, 1.0);

    vec3 colorResponse = vec3(fresnel * 0.1 + specular_value) + (diffuse) * vec3(0.3, 0.5, 0.8);
    FragColor = vec4(colorResponse, depth.x == 1.0 ? 0.0 : 1.0);
    float ar = exp(-10.0*(length(thickness - depth.xyz)));
    float ag = exp(-4.0*(length(thickness - depth.xyz)));
    float ab = exp(-2.0*(length(thickness - depth.xyz)));
    FragColor = vec4(vec3(fresnel * 0.1 + specular_value) + vec3(ar, ag, ab), 1.2 - depth.x);
}
