#version 450                                                                                                                                          
  
layout (location = 0) in vec2 var_tex0_coord;  
layout (set = 1, binding = 0) uniform sampler2D tex0_sampler;                                                     
layout (location = 0) out vec4 fragColor;
                                                     
void main(void) { 
    vec4 color = texture(tex0_sampler, var_tex0_coord);                                                                                    
    fragColor = vec4(color.r, 0.0, 0.0, 1.0);              
}
