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
float h = 0.01;
float offsetX;
float offsetY;
float gauss;
float grad_depth;

void main()
{
    vec3 depth = texture(depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 rev_depth = texture(rev_depth_tex_sampler, fragment.texture_uv).xyz;
    vec3 depth_color = vec3(0.0);
    vec3 rev_depth_color = vec3(0.0);
    float depth_alpha = 0.0;
    float sum=0.0;
    for(int i = 0; i < 5; i++){
      for(int j = 0; j < 5; j++){
        offsetX = (i/4.0 - 0.5) * 0.004;
        offsetY = (j/4.0 - 0.5) * 0.004;
        float stDevSquared = h*h;
        gauss = (1 / sqrt(2*3.14*stDevSquared)) * pow(2.71828, -((offsetX*offsetY)/(2*stDevSquared)));
        sum += gauss;
        depth_color += texture(depth_tex_sampler, fragment.texture_uv + vec2(offsetX, offsetY)).xyz * gauss;
        rev_depth_color += texture(rev_depth_tex_sampler, fragment.texture_uv + vec2(offsetX, offsetY)).xyz * gauss;
        depth_alpha += gauss * (texture(rev_depth_tex_sampler, fragment.texture_uv + vec2(offsetX, offsetY)).x - texture(depth_tex_sampler, fragment.texture_uv + vec2(offsetX, offsetY)).x + 0.01);
      }
    }
    depth_color /= sum;
    rev_depth_color /= sum;
    depth_alpha /= sum;

    float tex_d = texture(depth_tex_sampler, fragment.texture_uv + vec2(h, 0)).x ;
    float tex_g = texture(depth_tex_sampler, fragment.texture_uv + vec2(-h, 0)).x ;
    float tex_h = texture(depth_tex_sampler, fragment.texture_uv + vec2(0, h)).x ;
    float tex_b = texture(depth_tex_sampler, fragment.texture_uv + vec2(0, -h)).x ;
    grad_depth = ((tex_d - tex_g) + (tex_h-tex_b))/(2.0*h);

    if(depth_color.x >= 0.95){
      FragColor = vec4(vec3(1.0), 0.0);
    }else if(grad_depth >= 0.95 || grad_depth <= -0.95){
      FragColor = vec4(0.0, 0.5, 1.0, sqrt(depth_alpha)*2.2);
    }else{
      FragColor = vec4(0.0, 0.5, 1.0, sqrt(depth_alpha)*2.0);
      //FragColor = vec4(vec3(grad_depth), 1.0);
      //FragColor = vec4(0.0, 0.5, 1.0, grad_depth*0.5+0.1);
    }
}
