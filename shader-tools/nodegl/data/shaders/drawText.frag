#version 320 es                                                                                                                                             
precision highp float;  
  
layout (location = 0) in vec2 var_tex0_coord;  
layout (set = 1, binding = 0) uniform sampler2D tex0_sampler;                                                     
layout (location = 0) out vec4 fragColor;
                                                     
void main(void) {                                                                              
    fragColor = texture(tex0_sampler, var_tex0_coord);                     
}
