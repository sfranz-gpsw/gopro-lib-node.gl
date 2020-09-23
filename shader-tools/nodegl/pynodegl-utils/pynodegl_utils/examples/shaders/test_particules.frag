#version 450
precision mediump float;
layout (location = 0) out vec4 frag_color;

layout (std140, set = 2, binding = 0) uniform UBO_1 {
	vec4 color;
}; 

void main(void)
{
    frag_color = color;
}
