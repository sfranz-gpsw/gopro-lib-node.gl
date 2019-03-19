precision highp float;

in vec4 ngl_position;
in vec2 ngl_uvcoord;
in vec3 ngl_normal;
uniform mat4 ngl_modelview_matrix;
uniform mat4 ngl_projection_matrix;
uniform mat3 ngl_normal_matrix;

in vec4 instance_position;
in vec4 instance_color;
in float instance_scale;
in float instance_intensity;

out vec2 var_uvcoord;
out vec3 var_normal;
out vec4 var_color;
out vec3 var_frag_pos;
out float var_intensity;

void main()
{
    vec4 scale = vec4(instance_scale, instance_scale, instance_scale, 1.0);
    vec4 position = instance_position + scale * ngl_position;
    gl_Position = ngl_projection_matrix
                * ngl_modelview_matrix
                * position;
    var_frag_pos = vec3(ngl_modelview_matrix * position);
    var_uvcoord = ngl_uvcoord;
    var_normal = ngl_normal_matrix * ngl_normal;
    var_color = instance_color;
    var_intensity = instance_intensity;
}
