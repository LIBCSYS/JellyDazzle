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

# --- 039 butterfly lace: Temple Fay butterfly with drifting phase, x-mirror, jewel tones
BASE, WINDOW, SUB, TAU = 280, 1100, 38, 420
_BG = _vignette((0.04, 0.0, 0.06))

def render(t):
    tt = t + BASE
    n = np.arange(max(0, tt - WINDOW)*SUB, tt*SUB, dtype=np.float64)/SUB
    w = np.exp(-(tt - n)/TAU)
    th = n*0.16
    ph = n*0.0006
    r = 26.0*(np.exp(np.sin(th + 3*ph))
              - 2.0*np.cos(4*th + 5*ph)
              + np.sin((2*th - np.pi)/24.0)**5)
    x = r*np.sin(th)
    y = -r*np.cos(th) + 18.0
    hue = 0.75 + 0.18*np.sin(th*0.5 + 2*ph)
    col = _hsv(hue, 0.80, 1.0)
    acc = np.zeros((H, W, 3))
    for sx in (1.0, -1.0):
        _splat(acc, sx*x + CX, y + CY, col, w)
    return _finish(acc, 0.20, _BG)

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
