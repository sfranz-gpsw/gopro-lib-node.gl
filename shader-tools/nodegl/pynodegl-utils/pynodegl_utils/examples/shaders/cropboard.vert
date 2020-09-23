#version 450

precision highp float;

layout (location = 0) in vec3 ngl_position;
layout (location = 1) in vec2 ngl_uvcoord;
layout (location = 2) in vec2 uv_offset;
layout (location = 3) in vec2 translate_a;
layout (location = 4) in vec2 translate_b;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;   
	mat4 ngl_tex0_coord_matrix;  
	uniform float time;                                          
};                                                                                                

layout (location = 0) out vec2 var_tex0_coord;

void main(void)
{
    vec4 position = vec4(ngl_position,1.0) + vec4(mix(translate_a, translate_b, time), 0.0, 0.0);
    gl_Position = ngl_projection_matrix * ngl_modelview_matrix * position;
    var_tex0_coord = (ngl_tex0_coord_matrix * vec4(ngl_uvcoord + uv_offset, 0.0, 1.0)).xy;
}
