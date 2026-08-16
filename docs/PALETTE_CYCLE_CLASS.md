# The Dazzle method — a render-once, palette-cycled pattern class (2.5)

**J wants to see this.** It is the single most authentic thing we can add, and it is the
one architectural difference between JellyDazzle and the program it honours.

## The evidence

From the 83-second capture (`docs/dazzle_capture_palette_cycle.png`): **eight of twelve
sampled frames are the same composition.** Identical geometry — same bars, same wing
spikes, same corner ellipse clusters, pixel for pixel. Only the colour sweeps:
yellow/green → red/purple → magenta/cyan → green/cyan → blue/red.

Shiflett drew a frame **once** and then animated it for ten or fifteen seconds by
rotating the VGA DAC. No pixels were touched. On an 8088 that was the only way to get
motion that rich; the fact that it also looks *better* than redrawing is the lesson.

## What we do instead, today

Every layer repaints every pixel, every frame, at 60 fps. Our colour movement comes from
two places, and neither is what DAZZLE did:

- **scheme crossfade** — the palette's *contents* morph, so colours mutate in place
- **`g_prot` rotation** — the offset moves, so colour bands travel. As of 2.4.2 this has
  an idle floor so it never fully stops

Both act on geometry that is *also* moving. The original's colour swept across something
**still**, and that contrast is most of the effect.

## The class

A new pattern kind alongside `C_PURE` (repaint) and `C_CANVAS` (accumulator):
**`C_INDEXED`** — draws once, animates by palette alone.

### Contract

```c
/* Called ONCE per turn, at sl == 0. Writes a w*h byte plane of palette
 * indices — no colour, just indices. The engine does the rest. */
void pattern_NNN_index(uint8_t *idx, int w, int h, uint32_t seed);
```

Per frame, the engine maps `idx[]` through the live palette with the current offset:

```c
for (i = 0; i < w*h; i++)
    dst[i] = pal[(idx[i] + rot) & JD_PAL_MASK];
```

That is the whole per-frame cost: one table lookup per pixel, no math, no branches.
Cheaper than every pattern we ship.

### Why it cannot strobe

The frame-to-frame delta is bounded by how far `rot` moved times the ramp's local
gradient. A slow rotation over a smooth ramp is a **shear along the palette**, never a
jump — and unlike a repaint pattern there is no geometry change to add on top. This class
is strobe-free by construction, which is worth having after everything the 2.3/2.4 review
cycle cost us.

### What it unlocks

- **Genuine stillness.** A crystalline composition that holds while colour marches
  through it. We have never had this; every frame currently moves.
- **Density without cost.** The index plane can be as intricate as we like — packed line
  families, moiré, dense interference — because it is paid for once, not 60 times a
  second. This is also the natural home for the line-art primitives in `GAP_ANALYSIS.md`.
- **Per-line palette stepping** falls out for free: the index *is* the colour, so a
  primitive that steps its index by 1–17 per line produces DAZZLE's rainbow sweeps
  directly.

## Build order

1. `C_INDEXED` in the compositor: allocate the byte plane per layer, call the index
   function at `sl == 0`, add the map-through-palette blit. Nothing else changes.
2. One proof pattern — a **line-sweep string-art envelope**, the spec's workhorse. Two
   endpoint paths, N straight lines, index stepping per line, mirrored.
3. Look at it. If it holds up next to the smooth patterns, build the rest of the
   primitives against the same contract.
4. Only then consider whether rotation speed should differ per class — a still
   composition can take much faster cycling than a moving one without reading as busy.

## Open question worth settling early

Does a hard-edged, still, palette-cycled layer sit coherently *underneath* our smooth
flowing layers, or does it fight them? Two possible answers and both are fine:

- it composites beautifully and becomes a new GROUND family; or
- it needs its own **exclusive** mode — when an indexed layer runs, it runs alone, the
  way a `C_CANVAS` accumulator owns its turn.

Build step 2 before deciding. Do not guess.
