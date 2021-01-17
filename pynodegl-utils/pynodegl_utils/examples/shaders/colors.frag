vec3 linear_to_srgb(vec3 color)
{
    return mix(
        color * 12.92,
        1.055 * pow(color, vec3(1./2.4)) - .055,
        step(vec3(0.0031308), color)
    );
}

vec3 srgb_to_linear(vec3 color)
{
    return mix(
        color / 12.92,
        pow((color + .055) / 1.055, vec3(2.4)),
        step(vec3(0.04045), color)
    );
}

void main()
{
    vec3 color = ngl_texvideo(tex0, var_tex0_coord).rgb;

    /* gamma decode */
    color = srgb_to_linear(color);

    /* exposure */
    color *= colorize.rgb * exp2(exposure);
    //color = clamp(color, 0., 1.);

    /* saturation */
    float grey = dot(luma_weights, color);
    color = grey + saturation * (color - grey);
    //color = clamp(color, 0., 1.);

    /* contrast */
    color = .5 + contrast * (color - .5);

    // XXX: at which step do we apply this?
    color = clamp(color, 0., 1.);

    /* lift-gamma-gain */
    color = linear_to_srgb(color);
    color = pow(srgb_to_linear(color + lift.rgb * (1. - color)) * gain.rgb, 1. / gamma.rgb);

    /* gamma encode */
    color = linear_to_srgb(color);

    ngl_out_color = vec4(color, 1.0);
}
