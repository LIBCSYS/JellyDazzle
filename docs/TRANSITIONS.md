# Transitions — a 2.5 target

## The note

The original DAZZLE.EXE did not simply cross-fade between images. It **hopped between
its image-generation algorithms with a repertoire of transitions**: fades, dissolves,
scrolls, wraps and melts. The change itself was part of the show.

JellyDazzle currently has exactly one transition: a layer fades in over its neighbours
and fades out at the end of its turn. Every routine change looks identical. That is a
real gap against the thing we are paying homage to, and it is cheap to close.

## Sourcing note (read this before repeating the number)

Period descriptions and later write-ups commonly say Dazzle carried **roughly thirty
image-generation algorithms**. Treat that as the working figure, but know that it is
NOT what the primary source says. Shiflett's own shareware description reads:

> "The image engine has numerous primary image drawing algorithms, most of which have
> at least two styles of presentation, many of which have multiple internal drawing
> variations."

— [DAZZLE 5.0, archive.org](https://archive.org/details/msdos_festival_DAZZLE50)

He says *numerous*, not thirty, and the multiplier structure (algorithms x presentation
styles x internal variations) means any single count is a simplification. The Wikipedia
article gives no number at all. If we state a figure publicly, it needs a citation or a
hedge — see `site/tribute.html`, which now quotes him instead of asserting a count.

## What to build

Transitions are a property of the *handover* between two routines in a slot, not of the
routines themselves — so this lives in the compositor, not in the pattern plug-ins. Each
handover picks one from a bag, the same way routines are picked.

| Transition | Mechanism |
|---|---|
| **Fade** | what we have now: cross-dissolve the two layer buffers over N frames |
| **Dissolve** | per-pixel random threshold, ordered by a stable hash, so the new image arrives as noise that fills in |
| **Scroll** | the outgoing image slides off while the incoming slides on, any of four directions |
| **Wrap** | scroll where the outgoing wraps around the opposite edge, so the frame never shows empty space |
| **Melt** | per-column vertical drip: each column of the outgoing image falls at its own speed, revealing the incoming underneath |
| **Iris / wipe** | a shape (circle, diamond, N-fold star) opens from a point — fits the kaleidoscope idiom better than a straight wipe |

### Constraints, learned the hard way

- **Nothing may strobe.** Every transition must keep the frame-to-frame mean channel
  delta under the motion budget (< 8), same as everything else. A hard cut is banned.
- **Duration should scale with the layer's role.** A GROUND change is a big event and can
  take 2-3 s; a SPARK change should be quick.
- **Transitions must be interruptible** — a resize or a fullscreen toggle can land
  mid-transition and must not leave a layer half-composited.
- Melt and dissolve need a per-pixel state buffer; budget one extra `w*h` byte plane.

### Where it goes

`src/engine/compositor.c`, in the layer handover path — the same place that currently
ramps `peak` up and down. The bag machinery for picking a transition can reuse
`bag_draw()`.
