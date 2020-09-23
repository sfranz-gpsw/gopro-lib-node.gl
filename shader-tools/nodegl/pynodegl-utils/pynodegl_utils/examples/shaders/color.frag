#version 320 es
precision mediump float;

layout (std140, set = 1, binding = 0) uniform UBO_1 {
	vec4 color;
};
layout (location = 0) out vec4 fragColor;

void main(void)
{
    fragColor = color;
}
