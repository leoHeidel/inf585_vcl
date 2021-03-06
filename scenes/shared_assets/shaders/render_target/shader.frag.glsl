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
uniform mat4 view;
uniform mat4 perspective;

vec3 light = vec3(0.0, 0.0, 1.0); //vec3(0.0, 0.0, -2.0);

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

float maxDepth = 6.8;

vec3 getEyePos(sampler2D depthTex, vec2 texCoord){
  float a = texture(depthTex, texCoord).x;
  float depth = - (near + a * (far - near)); //getting back the depth in view space
  float Wx = (texCoord.x * 2.0 - 1.0) / fx;
  float Wy = (texCoord.y * 2.0 - 1.0) / fy;
  return vec3(-Wx, -Wy, 1.0) * depth;
}

float LinearizeDepth(float depth)
{
    return -(depth + near)/(far - near);
}

void main()
{
    vec4 depth = texture(depth_tex_sampler, fragment.texture_uv);
    if(depth.x > 0.99){
      discard;
    }else{
      vec3 thickness = texture(rev_depth_tex_sampler, fragment.texture_uv).xyz;
      vec3 posEye = getEyePos(depth_tex_sampler, fragment.texture_uv);
      light += vec3(0.0, 0.0, -length(camera_position)); //make light still in camera's frame

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
      n = cross(ddx, ddy);
      n = normalize(n);
      u = -normalize(light - posEye);
      r = reflect(u,n);
      t = normalize(vec3(0.0) - posEye); //camera's at origin in camera's frame

      // Rendering characteristics
      float specular_value = 2.0 * pow(clamp(dot(r,t), 0.0, 1.0), 32);
      float fresnel = clamp(R0 + (1.0 - R0)*pow(1-dot(n,t), 5.0), 0.0, 1.0);
      float diffuse = clamp(dot(u,n), 0.0, 1.0);

      // Beer's law of absorption
      float ar = exp(-5.0*(thickness.x));
      float ag = exp(-2.0*(thickness.x));
      float ab = exp(-1.0*(thickness.x));
      //+ vec3(ar, ag, ab) // fresnel * 0.1 +  // 1.2 - depth.x
      FragColor = vec4(vec3(fresnel * 0.05 + specular_value) + vec3(ar, ag, ab), 1.0 - ar);
      //FragColor = vec4(vec3(depth.x), 1.0);
    }
}
