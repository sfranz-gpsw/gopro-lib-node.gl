precision mediump float;

in vec3 var_frag_pos;
in vec4 var_color;
in vec3 var_normal;
in float var_intensity;

out vec4 frag_color[2];

void main(void)
{
    vec3 light_color = vec3(1.0);
    float ambient_strenght = 0.1;
    vec3 ambient = ambient_strenght * light_color;

    vec3 light_pos = vec3(4.0, 1.0, 4.0);
    vec3 light_dir = normalize(light_pos - var_frag_pos);
    vec3 diffuse = max(dot(var_normal, light_dir), 0.0) * light_color;

    float luma = var_color.r * 0.2126 + var_color.g * 0.7152 + var_color.b * 0.0722;
    frag_color[0] = vec4(var_color.rgb * (ambient + diffuse), 1.0);
    if (luma > 0.8)
        frag_color[1] =  var_color;
    else
        frag_color[1] =  vec4(0.0, 0.0, 0.0, 1.0);
}
