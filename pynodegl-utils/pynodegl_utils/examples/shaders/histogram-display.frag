void main()
{
    /* find a scale where outliers are excluded */
    float exclude = 0.1; // 10% is the interdecile range (IDR)
    uint l = uint(exclude * 255.);
    uint r = 255 - l;
    vec3 q1 = vec3(shist.r[l], shist.g[l], shist.b[l]);
    vec3 q3 = vec3(shist.r[r], shist.g[r], shist.b[r]);
    vec3 ikr = q3 - q1;
    vec3 upper = q3 + 1.5 * ikr;
    float maximum = max(max(upper.r, upper.g), upper.b);
    float scale = 1. / maximum;

    /* load histogram data */
    uint x = clamp(uint(var_uvcoord.x * 255.0 + 0.5), 0U, 255U);
    vec3 rgb = clamp(vec3(hist.r[x], hist.g[x], hist.b[x]) * scale, 0., 1.);

    float y = var_uvcoord.y;
    vec3 ymax = vec3(1.0/3.0, 2.0/3.0, 1.0);
    vec3 v = rgb * 1.0/3.0;
    vec3 yzero = ymax - v;
    vec3 yval = step(y, ymax) * (1.0 - step(y, yzero)); // y <= ymax && y > yzero
    ngl_out_color = vec4(yval, alpha);
}
