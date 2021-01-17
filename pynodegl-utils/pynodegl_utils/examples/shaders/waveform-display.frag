vec3 mix3(vec3 c0, vec3 c1, vec3 c2, float x)
{
    return mix(mix(c0, c1, x*2.), mix(c1, c2, x*2.-1.), step(.5, x));
}

void main()
{
    /* load histogram data */
    float x = var_uvcoord.x;
    float y = var_uvcoord.y;

    vec3 filt = vec3(
        x >= 0./3. && x < 1./3. ? 1. : 0.,
        x >= 1./3. && x < 2./3. ? 1. : 0.,
        x >= 2./3. && x < 3./3. ? 1. : 0.
    );

    if (parade) {
        vec3 color_split = x*3. - vec3(0., 1., 2.);
        x = dot(filt, color_split);
    //} else {
    //    filt = vec3(1.);
    }

    uint xdata = clamp(uint(x * float(max_width)), 0U, max_width);
    uint ydata = clamp(uint((1. - y) * 255.0), 0U, 255U);
    uint idx = xdata * 256 + ydata;
    vec3 rgb = vec3(hist.r[idx], hist.g[idx], hist.b[idx]);
    float luma = float(hist.y[idx]);

    /* find a scale where outliers are excluded */
    float exclude = 0.1; // 10% is the interdecile range (IDR)
    // XXX: need to be done on global histogram
    uint l = xdata*256 + uint(exclude * 255.);
    uint r = xdata*256 + 255 - l;
    vec3 q1 = vec3(shist.r[l], shist.g[l], shist.b[l]);
    vec3 q3 = vec3(shist.r[r], shist.g[r], shist.b[r]);
    vec3 ikr = q3 - q1;
    vec3 upper = q3 + 1.5 * ikr;
    float maximum = max(max(upper.r, upper.g), upper.b);
    float scale = 1. / maximum;

    //rgb = log(rgb) / log(10.f);

    if (parade)
        rgb *= filt;

    // XXX
    //float scale = 0.1f;
    //float scale = 1.f / float(hist.max_rgb);
    //float scale_luma = 1.f / float(hist.max_luma);

    // gamma compress: the histogram is represented with an amount of light,
    // so to have a linear perception of it, we apply a simplified linear to
    // sRGB convert (actually just a gamma 2.2 power) because the output is
    // expected to be non-linear. The range of the value stays the same (0 to
    // 1), but darker values will get brighten up.
    vec3 intensity = pow(rgb * scale, vec3(1./2.2));

    //intensity *= sqrt(2.);

    float off = .3;
    vec3 color;
    if (parade) {
        float k = dot(intensity, vec3(1.));

        // use "desaturated" red/green/blue base colors
        vec3 base_color = min(filt + off, vec3(1.));

        // interpolate on the gradient:
        // black -> base color -> white
        color = mix3(vec3(0.), base_color, vec3(1.), k);
    } else {
        vec3 base_color_r = vec3(1., off, off);
        vec3 base_color_g = vec3(off, 1., off);
        vec3 base_color_b = vec3(off, off, 1.);

        vec3 color_r = mix3(vec3(0.), base_color_r, vec3(1.), intensity.r);
        vec3 color_g = mix3(vec3(0.), base_color_g, vec3(1.), intensity.g);
        vec3 color_b = mix3(vec3(0.), base_color_b, vec3(1.), intensity.b);

        //vec3 color_y = vec3(pow(luma * scale_luma, 1./2.2));
        //color = (color_r + color_b + color_g + color_y) / 4.;

        color = (color_r + color_b + color_g) / 3.;
    }

    ngl_out_color = vec4(color, alpha);
}
