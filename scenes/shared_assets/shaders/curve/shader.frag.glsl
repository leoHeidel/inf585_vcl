#version 330 core


out vec4 FragColor;

uniform vec3 color;
uniform float color_alpha;


void main()
{
    FragColor = vec4(color, color_alpha);
}
