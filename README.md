# dazzle64 — step 1: circle in a square

## One-time setup

```bash
brew install sdl2
```

## Build & run

```bash
clang main.c draw.s -o dazzle64 $(sdl2-config --cflags --libs)
./dazzle64
```

Esc or close the window to quit.

That's the whole toolchain — clang assembles `draw.s` natively (it's the
same assembler Xcode uses), links it with the C shim, done. No Makefile
needed yet.

## The mental model (mode 13h → 2026)

| DOS / dazzle.exe            | This project                          |
|-----------------------------|---------------------------------------|
| INT 10h, AX=0013h           | `main.c` (SDL window + texture)       |
| A000:0000 framebuffer       | `framebuffer[]` array in main.c       |
| 320×200, 1 byte/pixel       | 800×600, 4 bytes/pixel (ARGB)         |
| `mov es:[di], al`           | `str w11, [x0, w13, uxtw #2]`         |
| offset = y*320 + x          | offset = y*width + x (same idea)      |

## Where the kaleidoscope comes from

The per-pixel loop in draw.s is already the right skeleton: for each
pixel you compute (dx, dy) from the center. A kaleidoscope is just a
transform on (dx, dy) before you decide the color — mirror it into one
wedge (fold the angle), sample a pattern, and every pixel with the same
folded coordinates gets the same color. Add a frame counter parameter
and rotate the fold each frame, and it moves.
