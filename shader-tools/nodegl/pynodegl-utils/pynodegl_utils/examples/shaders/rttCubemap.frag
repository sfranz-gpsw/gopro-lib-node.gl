#version 450
precision highp float;
layout (location = 0) out vec4 frag_color[6];

void main()
{
    frag_color[0] = vec4(1.0, 0.0, 0.0, 1.0); // right
    frag_color[1] = vec4(0.0, 1.0, 0.0, 1.0); // left
    frag_color[2] = vec4(0.0, 0.0, 1.0, 1.0); // top
    frag_color[3] = vec4(1.0, 1.0, 0.0, 1.0); // bottom
    frag_color[4] = vec4(0.0, 1.0, 1.0, 1.0); // back
    frag_color[5] = vec4(1.0, 0.0, 1.0, 1.0); // front
}
