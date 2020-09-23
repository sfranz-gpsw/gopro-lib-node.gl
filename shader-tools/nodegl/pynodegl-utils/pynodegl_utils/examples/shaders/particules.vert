#version 450
out gl_PerVertex {
    vec4 gl_Position;   
};
precision highp float;

layout (location = 0) in vec4 ngl_position;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;
};  

layout(std430, set = 1, binding = 0) readonly buffer SSBO_0 {
    vec3 positions[];
};

void main(void)
{
    vec4 position = ngl_position + vec4(positions[gl_InstanceIndex], 0.0);
    gl_Position = ngl_projection_matrix * ngl_modelview_matrix * position;
}
