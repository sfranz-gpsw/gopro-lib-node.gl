#version 320 es
precision highp float;

layout (location = 0) in vec3 var_normal;
layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(var_normal, 1.0);
}
