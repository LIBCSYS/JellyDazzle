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

# --- 038 guilloche rings: 5 concurrent engine-turned bands, phases crawl, metallic palette
BASE, WINDOW, SUB, TAU = 260, 900, 40, 380
_R0 = np.array([34.0, 52.0, 68.0, 84.0, 100.0])
_HUE = np.array([0.11, 0.05, 0.48, 0.14, 0.58])
_SAT = np.array([0.85, 0.90, 0.80, 0.35, 0.80])
_BG = _vignette((0.04, 0.03, 0.01))

def render(t):
    tt = t + BASE
    n = np.arange(max(0, tt - WINDOW)*SUB, tt*SUB, dtype=np.float64)/SUB
    w = np.exp(-(tt - n)/TAU)
    idx = (np.arange(n.size) % 5)
    R0 = _R0[idx] + 4.0*np.sin(n*0.0008 + idx)
    th = n*0.37
    r = (R0 + 9.0*np.sin(11*th + n*0.0014*(idx + 1))
            + 5.0*np.sin(17*th - n*0.0009))
    x = r*np.cos(th)
    y = r*np.sin(th)
    hue = _HUE[idx] + 0.02*np.sin(n*0.0005 + idx*2.0)
    col = _hsv(hue, _SAT[idx], 1.0)
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
