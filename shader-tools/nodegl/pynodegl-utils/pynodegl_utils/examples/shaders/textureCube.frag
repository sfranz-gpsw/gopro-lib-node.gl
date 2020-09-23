#version 320 es                                                                                                                                             
precision highp float;
layout (location = 0) in vec3 var_uvcoord;
layout (location = 0) out vec4 frag_color;
layout (set = 1, binding = 0) uniform samplerCube tex0_sampler;

void main()
{
    frag_color = texture(tex0_sampler, vec3(var_uvcoord.xy, 0.5));
}
