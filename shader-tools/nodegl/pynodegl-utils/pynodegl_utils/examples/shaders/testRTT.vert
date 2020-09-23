#include "ngl_common.vert.h"
                                                        
layout(location = 0) in vec3 ngl_position;                                                      
layout(location = 1) in vec3 ngl_normal; 

layout (std140, set = 0, binding = 0) uniform UBO_0 {
	mat4 ngl_modelview_matrix;                                                
	mat4 ngl_projection_matrix;                                               
	mat4 ngl_normal_matrix;                                                   
	mat4 ngl_tex0_coord_matrix;
};                                                                                                

layout(location = 0) out vec3 var_normal;                                                          

void main() {                                                                                 
    setNglPos(ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0));    
    var_normal = mat3(ngl_normal_matrix) * ngl_normal;                                  
}
