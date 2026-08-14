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

def _rot(x, y, a):
    c, s = np.cos(a), np.sin(a)
    return x*c - y*s, x*s + y*c

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

# --- 033 rose engine: offset rhodonea with drifting petal count, 7-fold dihedral, warm
BASE, WINDOW, SUB, TAU = 260, 950, 32, 360
_BG = _vignette((0.07, 0.02, 0.01))

def render(t):
    tt = t + BASE
    n = np.arange(max(0, tt - WINDOW)*SUB, tt*SUB, dtype=np.float64)/SUB
    w = np.exp(-(tt - n)/TAU)
    th = n*0.19
    k = 2.5 + 1.5*np.sin(n*0.0006)                 # petal count drifts slowly
    r = (76 + 8*np.sin(n*0.0013))*np.cos(k*th) + 18*np.sin(n*0.0004)
    x = r*np.cos(th)
    y = r*np.sin(th)
    hue = 0.93 + 0.24*np.clip((r + 94.0)/188.0, 0, 1) + 0.02*np.sin(n*0.0005)  # crimson core -> gold rim
    col = _hsv(hue, 0.88, 1.0)
    acc = np.zeros((H, W, 3))
    for kk in range(7):
        a = kk*2.0*np.pi/7.0
        for sy in (1.0, -1.0):
            rx, ry = _rot(x, y*sy, a)
            _splat(acc, rx + CX, ry + CY, col, w)
    return _finish(acc, 0.07, _BG)

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
