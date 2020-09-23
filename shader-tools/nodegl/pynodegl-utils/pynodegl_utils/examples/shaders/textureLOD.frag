#version 450
precision highp float;
layout (location = 0) in vec2 var_uvcoord;
layout (location = 0) out vec4 frag_color;
layout (set = 1, binding = 0) uniform sampler2D tex0_sampler;

void main(void)
{
    frag_color = textureLod(tex0_sampler, var_uvcoord, 0.5);
}
