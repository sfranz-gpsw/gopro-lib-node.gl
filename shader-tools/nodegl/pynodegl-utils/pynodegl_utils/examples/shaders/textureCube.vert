#include "ngl_common.vert.h"
precision highp float;
layout(location = 0) in vec3 ngl_position;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;
};

layout(location = 0) out vec3 var_uvcoord;

void main()
{
    setNglPos(ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0));
    var_uvcoord = ngl_position;
}
