#include "ngl_common.vert.h"
precision highp float;
layout (location = 0) in vec3 ngl_position;
layout (location = 1) in vec4 matrix_0;
layout (location = 2) in vec4 matrix_1;
layout (location = 3) in vec4 matrix_2;
layout (location = 4) in vec4 matrix_3;

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;
};   

void main() {
	mat4 matrix = mat4(matrix_0, matrix_1, matrix_2, matrix_3);
    setNglPos(ngl_projection_matrix * ngl_modelview_matrix * matrix * vec4(ngl_position, 1.0));
}
