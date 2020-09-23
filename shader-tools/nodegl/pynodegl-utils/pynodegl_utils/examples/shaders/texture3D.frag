#version 320 es                                                                                                                                             
precision highp float;
precision mediump sampler3D;
layout (location = 0) in vec2 var_uvcoord;
layout (location = 0) out vec4 frag_color;
layout (set = 1, binding = 0) uniform sampler3D tex0_sampler;

void main()
{
    frag_color = texture(tex0_sampler, vec3(var_uvcoord, 0.0));
    frag_color += texture(tex0_sampler, vec3(var_uvcoord, 0.5));
    frag_color += texture(tex0_sampler, vec3(var_uvcoord, 1.0));
}
