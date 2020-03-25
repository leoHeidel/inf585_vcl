#version 330 core

in struct fragment_data
{
    vec4 position;
    vec4 normal;
    vec4 color;
    vec2 texture_uv;
} fragment;

uniform sampler2D texture_sampler;
uniform mat4 view;

out vec4 FragColor;

uniform vec3 camera_position;
uniform vec3 color     = vec3(1.0, 1.0, 1.0);
uniform float color_alpha = 1.0;
uniform float ambiant  = 0.2;
uniform float diffuse  = 0.8;
uniform float specular = 0.5;
uniform int specular_exponent = 128;
uniform bool isBack;

vec3 light = vec3(camera_position.x, camera_position.y, camera_position.z);

void main()
{
    vec3 n = normalize(fragment.normal.xyz);
    vec3 u = normalize(light-fragment.position.xyz);
    vec3 r = reflect(u,n);
    vec3 t = normalize(fragment.position.xyz-camera_position);


    float diffuse_value  = diffuse * clamp( dot(u,n), 0.0, 1.0);
    float specular_value = specular * pow( clamp( dot(r,t), 0.0, 1.0), specular_exponent);


    vec3 white = vec3(1.0);
    vec4 color_texture = texture(texture_sampler, fragment.texture_uv);
    vec3 c = (ambiant+diffuse_value)*color.rgb*fragment.color.rgb*color_texture.rgb + specular_value*white;
    bool isDot = isBack ? (dot(n,t) < 0) : (dot(n,t) >= 0);
    if(isDot){
      discard;
    }else{
      float eps = -(view * fragment.position).z / (view * fragment.position).w * 0.0003;
      if(fragment.texture_uv.x > eps && fragment.texture_uv.x < 1.0-eps && fragment.texture_uv.y > eps && fragment.texture_uv.y < 1.0-eps){
        if(isBack){
          float cx = floor(6.0 * fragment.texture_uv.x);
          float cy = floor(6.0 * fragment.texture_uv.y);
          float result = mod(cx + cy, 2.0);
          float c = mix(1.0, 0.0, sign(result));
          FragColor = vec4(vec3(c), 1-c);
        }else{
          discard;
        }
      }else{
        float e = min(min(fragment.texture_uv.x, 1.0 - fragment.texture_uv.x),min(fragment.texture_uv.y, 1.0 - fragment.texture_uv.y));
        FragColor = vec4(0.0, 0.0, 0.0, 1.0 - e/eps);
      }
    }
    //FragColor = vec4(c, dot(n,t));
}
