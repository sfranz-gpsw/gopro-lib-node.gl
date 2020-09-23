#version 450
layout (location = 0) in vec3 var_normal;
layout (location = 0) out vec4 fragColor;

void main(void)
{
    fragColor = vec4((var_normal + 1.0) * 0.5, 1.0);
}
