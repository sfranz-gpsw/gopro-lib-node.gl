#version 100

precision highp float;

attribute vec4 ngl_position;
attribute vec2 ngl_uvcoord;

uniform mat4 tex0_coord_matrix;
uniform vec2 tex0_dimensions;

varying vec2 var_blur_coord[5];

void main()
{
    gl_Position = ngl_position;
    vec2 off = 1.0 / tex0_dimensions;
    vec2 tex_coord = (tex0_coord_matrix * vec4(ngl_uvcoord, 0.0, 1.0)).xy;
    var_blur_coord[0] = tex_coord;
    var_blur_coord[1] = tex_coord + off * 1.407333;
    var_blur_coord[2] = tex_coord - off * 1.407333;
    var_blur_coord[3] = tex_coord + off * 3.294215;
    var_blur_coord[4] = tex_coord - off * 3.294215;
}
