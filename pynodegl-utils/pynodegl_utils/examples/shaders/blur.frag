#version 100

precision highp float;

uniform sampler2D tex0_sampler;
varying vec2 var_blur_coord[5];

void main()
{
    lowp vec4 sum = vec4(0.0);
    sum += texture2D(tex0_sampler, var_blur_coord[0]) * 0.204164;
    sum += texture2D(tex0_sampler, var_blur_coord[1]) * 0.304005;
    sum += texture2D(tex0_sampler, var_blur_coord[2]) * 0.304005;
    sum += texture2D(tex0_sampler, var_blur_coord[3]) * 0.093913;
    sum += texture2D(tex0_sampler, var_blur_coord[4]) * 0.093913;
    gl_FragColor = sum * 1.5;
}
