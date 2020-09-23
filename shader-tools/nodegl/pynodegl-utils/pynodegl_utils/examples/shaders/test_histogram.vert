#version 450
out gl_PerVertex {
    vec4 gl_Position;   
};
layout (location = 0) in vec3 ngl_position;
layout (location = 1) in vec2 ngl_uvcoord;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;
};  

layout (location = 0) out vec2 var_uvcoord;

void main()
{
    gl_Position = ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0);
    var_uvcoord = ngl_uvcoord;
}
