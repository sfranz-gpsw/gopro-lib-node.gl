#version 450
#define H_SIZE 64
#define SIZE 8
precision highp float;
layout (location = 0) in vec2 var_uvcoord;
layout (location = 0) out vec4 frag_color;

layout(std430, set = 1, binding = 0) readonly buffer histogram {
    uint histr[H_SIZE];
    uint histg[H_SIZE];
    uint histb[H_SIZE];
    uint max_r, max_g, max_b;
};

void main()
{
    uint x = uint(var_uvcoord.x * float(SIZE));
    uint y = uint(var_uvcoord.y * float(SIZE));
    uint i = clamp(x + y * uint(SIZE), 0U, uint(H_SIZE) - 1U);
    vec3 rgb = vec3(histr[i], histg[i], histb[i]) / vec3(max_r, max_g, max_b);
    frag_color = vec4(rgb, 1.0);
}
