#version 330 core

// Shader use: final render of the fluid

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
uniform mat3 rotation;

vec3 light = vec3(0.0, -2.0, -2.0);

float near = 3.0;
float far  = 10.0;
float angle_of_view = 50*3.14159f/180;
float fy = 1/tan(angle_of_view/2);
float fx = fy/1.28f;
float texelSize = 0.002;

float R0 = 0.5;
vec3 n; // normal
vec3 u; // light direction
vec3 r; // light reflection
vec3 t; // view direction


vec3 getEyePos(sampler2D depthTex, vec2 texCoord){
  float a = texture(depthTex, texCoord).x;
  float depth = - (near + a * (far - near)); //getting back the depth in view space
  float Wx = (texCoord.x * 2.0 - 1.0) / fx;
  float Wy = (texCoord.y * 2.0 - 1.0) / fy;
  return vec3(Wx, Wy, 1.0) * depth;
}

float LinearizeDepth(float depth)
{
    return -(depth + near)/(far - near);
}

void main()
{
    vec4 depth = texture(depth_tex_sampler, fragment.texture_uv);
    vec3 thickness = texture(rev_depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 posEye = getEyePos(depth_tex_sampler, fragment.texture_uv);

    // Compute small change of position along x axis
    vec3 ddx = getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(texelSize, 0)) - posEye;
    vec3 ddx2 = posEye - getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(-texelSize, 0));
    if (abs(ddx.z) > abs(ddx2.z)) {
      ddx = ddx2;
    }

    // Compute small change of position along y axis
    vec3 ddy = getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(0, texelSize)) - posEye;
    vec3 ddy2 = posEye - getEyePos(depth_tex_sampler, fragment.texture_uv + vec2(0, -texelSize));
    if (abs(ddy.z) > abs(ddy2.z)) {
      ddy = ddy2;
    }

    // Compute normal from cross product
    n = cross(ddy, ddx);
    n = rotation * normalize(n);
    u = normalize(light-fragment.position.xyz);
    r = reflect(u,n);
    t = normalize(fragment.position.xyz-camera_position);

    // Rendering characteristics
    float specular_value = 0.5 * pow(clamp(dot(r,t), 0.0, 1.0), 128);
    float fresnel = clamp(R0 + (1.0 - R0)*pow(1-dot(n,t), 5.0), 0.0, 1.0);
    float diffuse = clamp(dot(normalize(light-fragment.position.xyz),n), 0.0, 1.0);

    // Beer's law of absorption
    float ar = exp(-10.0*(length(thickness - depth.xyz)));
    float ag = exp(-4.0*(length(thickness - depth.xyz)));
    float ab = exp(-2.0*(length(thickness - depth.xyz)));

    FragColor = vec4(vec3(fresnel * 0.1 + specular_value) + vec3(ar, ag, ab), 1.2 - depth.x);
}
