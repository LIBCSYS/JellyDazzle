# 088 Star Cross

## Look
The classic Islamic star-and-cross tiling as living objects: gold-rimmed 8-point stars
filled with concentric rainbow layers alternate with cool plus-shaped crosses on a dark
indigo weave, each shape breathing on its own phase while the whole tiling drifts.

## Math
- Checkerboard lattice pitch L=76: nearest of the two offset square lattices chosen by
  Euclidean distance; sublattice parity decides star vs cross.
- 8-point star metric: `d8 = min(max(|u|,|v|), (|u|+|v|)/√2)` — union of an axis
  square and its 45°-rotated twin.
- Star radius breathes: `Rs = 27 + 4·sin(0.011t + 2π·h)` (h = per-cell hash);
  interior hue = 0.45·d8/Rs + 0.35·h + 0.0013·t, layered by sin(0.35·d8 − 0.014t).
- Cross: min(|u|,|v|) < 10 + 2.5·sin(0.011t + 3 + 2πh), clipped cheb < 30.
- Rim where |d8 − Rs| < 1.6.

## Integer ARM64 plan
- Cell lookup: with L a power of two (64), nearest-lattice = shifts/ANDs on x and
  x−L/2 plus one manhattan compare (Euclidean vs manhattan choice differs only in
  corner slivers — octagonal compare is fine).
- d8 = min/max of |u|,|v| adds; the /√2 is a multiply by 181 >> 8.
- Per-cell hash from lattice coords (xorshift byte); breathing radius = per-CELL
  scalar (few dozen cells/frame) from the sine table, not per-pixel.
- Fill bands via compares; hue index = (d8·k)/Rs by one reciprocal-table multiply.
- Tiling drift = add Q8.8 offset to x,y before shifts.

## Palette pairing
Stars sweep the warm half of a cosine rainbow (each star anchored by its hash hue),
crosses hold the cyan-blue band, ground is near-black indigo weave, star rims pale gold
— warm objects / cool objects / dark field: three clean layers.

## Motion
Stars and crosses breathe on ~9.5 s periods with random per-cell phase (never in sync,
never all still); interior layers ripple outward; the whole tiling drifts diagonally at
0.03 px/frame; global hue rolls over ~13 s. Gentle, ornamental, continuous.
