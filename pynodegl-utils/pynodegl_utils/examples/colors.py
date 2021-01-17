import pynodegl as ngl
from pynodegl_utils.misc import scene


def _canvas(cfg, media, **params):
    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(media.filename)
    t = ngl.Texture2D(data_src=m, mag_filter='linear', min_filter='linear')
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('colors'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    r = ngl.Render(q, p)
    r.update_frag_resources(tex0=t, **params)
    return r


@scene(
    colorize=scene.Color(),
    exposure=scene.Range(range=[-2, 2], unit_base=1000),
    saturation=scene.Range(range=[0, 2], unit_base=1000),
    contrast=scene.Range(range=[0, 2], unit_base=1000),
    lift=scene.Color(),
    gamma=scene.Color(),
    gain=scene.Color(),
)
def colors(
    cfg,
    colorize=(1, 1, 1, 0),
    exposure=0,
    saturation=1,
    contrast=1,
    lift=(0, 0, 0, 1),
    gamma=(1, 1, 1, 1),
    gain=(1, 1, 1, 1)
):
    m = cfg.medias[0]
    cfg.duration = m.duration
    cfg.aspect_ratio = (m.width, m.height)


    c = _canvas(cfg, m,
        luma_weights=ngl.UniformVec3(value=(.2126, .7152, .0722)),  # BT.709
        colorize=ngl.UniformVec4(colorize),
        exposure=ngl.UniformFloat(exposure),
        saturation=ngl.UniformFloat(saturation),
        contrast=ngl.UniformFloat(contrast),
        lift=ngl.UniformVec4(lift),
        gamma=ngl.UniformVec4(gamma),
        gain=ngl.UniformVec4(gain),
    )

    return c
