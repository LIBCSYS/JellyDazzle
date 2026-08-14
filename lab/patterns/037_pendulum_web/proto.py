import numpy as np, os, subprocess

W, H = 320, 240
CX, CY = W*0.5, H*0.5

def _hsv(h, s, v):
    h6 = np.mod(h, 1.0)*6.0
    i = h6.astype(np.int32) % 6
    f = h6 - np.floor(h6)
    one = np.ones_like(f)
    s = s*one; v = v*one
    p = v*(1-s); q = v*(1-s*f); u = v*(1-s*(1-f))
    r = np.choose(i, [v, q, p, p, u, v])
    g = np.choose(i, [u, v, v, q, p, p])
    b = np.choose(i, [p, p, u, v, v, q])
    return np.stack([r, g, b], axis=-1)

def _splat(acc, x, y, col, w):
    xi = np.round(x).astype(np.int32); yi = np.round(y).astype(np.int32)
    m = (xi >= 0) & (xi < W) & (yi >= 0) & (yi < H)
    np.add.at(acc, (yi[m], xi[m]), col[m]*w[m][:, None])

def _vignette(col, k=1.0):
    yy, xx = np.mgrid[0:H, 0:W]
    d = ((xx-CX)/W)**2 + ((yy-CY)/H)**2
    return (np.exp(-d*3.0)*k)[:, :, None]*np.array(col)

def _finish(acc, gain, bg=None):
    a = acc
    for ax in (0, 1):
        a = 0.5*a + 0.25*(np.roll(a, 1, axis=ax) + np.roll(a, -1, axis=ax))
    img = 1.0 - np.exp(-a*gain)
    if bg is not None:
        img = img + (1.0 - img)*bg
    return (np.clip(img, 0.0, 1.0)**0.85*255.0).astype(np.uint8)

# --- 037 pendulum web: rungs strung between two precessing Lissajous curves, 180-degree symmetry
BASE, RPF, WRU, TAU, S = 280, 3, 2400, 800, 40
_BG = _vignette((0.03, 0.0, 0.05))

def render(t):
    tt = t + BASE
    mtot = tt*RPF
    m = np.arange(max(0, mtot - WRU), mtot, dtype=np.float64)
    w = np.exp(-(mtot - m)/TAU)
    s = m*0.213
    ph = m*0.0009
    x1 = 105*np.sin(2.0*s + ph);        y1 = 80*np.sin(3.0*s)
    x2 = 105*np.sin(3.0*s + 1.2 - ph);  y2 = 80*np.sin(5.0*s + 0.5)
    u = np.linspace(0.0, 1.0, S)
    x = (x1[:, None]*(1-u) + x2[:, None]*u).ravel()
    y = (y1[:, None]*(1-u) + y2[:, None]*u).ravel()
    hue = (0.55 + tt*0.0004 + 0.0*m)[..., None] + u*0.35   # two-tone rung: cyan end -> magenta end
    col = _hsv(np.broadcast_to(hue, (m.size, S)).ravel(), 0.90, 1.0)
    ww = np.repeat(w, S)
    acc = np.zeros((H, W, 3))
    for sgn in (1.0, -1.0):
        _splat(acc, sgn*x + CX, sgn*y + CY, col, ww)
    return _finish(acc, 0.26, _BG)

def _main():
    strip = np.concatenate([render(0), render(300), render(700)], axis=1)
    with open('/tmp/x.ppm', 'wb') as f:
        f.write(b'P6\n%d %d\n255\n' % (strip.shape[1], strip.shape[0]))
        f.write(np.ascontiguousarray(strip).tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'preview.png')
    subprocess.run(['sips', '-s', 'format', 'png', '/tmp/x.ppm', '--out', out],
                   check=True, capture_output=True)

if __name__ == '__main__':
    _main()
