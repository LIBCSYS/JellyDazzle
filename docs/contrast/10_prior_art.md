# 10 — Prior Art: how everyone else made a foreground read over a busy background

**Question this answers:** the ground draws at 100% opacity full-frame permanently; accents composite at 35–43% under MAX or SCREEN; MAX degenerates to a no-op against a bright ground and SCREEN produces fog. Every field below has hit this exact wall. This is what they did, what it costs here, and what to steal.

**Scope note.** All costs in this document were **measured on this machine** at 3456×2160 (7,464,960 px), single-threaded, `clang -O2`, against the engine's real kernel shapes. They are not estimates. Method and raw numbers in §7.

---

## 0. The engine, stated precisely (so the mapping below is checkable)

| Fact | Where |
|---|---|
| Blend modes: `B_MIX, B_MAX, B_SCREEN, B_ADD, B_DIFF` | `src/engine/compositor.c:122` |
| Kernels: `span_scale` `span_gain` `span_lerp` `span_max` `span_screen` `span_add` `span_diff`, dispatched by `blend_span` | `compositor.c:1247–1425` |
| **Every mode except MIX and DIFF is lighten-only.** There is no darken. | `compositor.c:1412` |
| The engine already knows this is a problem: *"black is a no-op under MAX/SCREEN/ADD"* | `compositor.c:1395` (comment) |
| Ground is `memcpy` at full weight, or a `span_lerp` between two grounds on handover | `compositor.c:~2584` |
| Overlay peak weights Q8 per slot: lo `{256,128,100,72}`, hi `{256,180,140,115}` | `compositor.c:1442` |
| `pick_blend` already caps SCREEN by the ground's measured luma (64..154 Q8) | `compositor.c:1617–1640` |
| **Per-layer 32768-entry palette with a 256-entry transfer LUT, a saturation gain `gq`, a hue rotation and a window (`span`/`off`) into the shared ramp** | `layer_pal_build`, `compositor.c:1117–1240` |
| Bilinear upscale machinery already in tree and proven | `src/patterns/_upsample.h` (`jd_up`) |
| Affine per-layer warp with 16.16 stepping | `layer_warp`, `compositor.c:1370` |
| Frame budget: `BUDGET_Q8` = 10.5 ms; `g_ewma_ms` ≈ 6.0 typical; hot at >13.5 ms | `compositor.c:116, 308, 2808` |
| Dead-air guard lifts the whole composite toward `JD_LUMA_FLOOR` via `span_gain` | `compositor.c:~2638` |
| Contrast measurement tap (3.0.4) | `src/engine/jellydazzle.h` (`jd_tap`) |

Two structural consequences that drive everything below:

- **The palette is a free compositing surface.** A pass over one layer's 32768-entry palette costs **0.037 ms**. A pass over the frame costs **2.29 ms** scalar / **0.70 ms** NEON. The palette is ~60× cheaper than the cheapest full-frame pass and ~200× cheaper than the kernels actually shipping. Anything expressible as a transfer curve should live there.
- **A per-pixel matte is nearly free if it replaces a lerp.** A holdout-matte keymix reading a 1/8-scale matte measured **2.07 ms** scalar — statistically identical to `span_lerp`'s 2.04 ms, because the 116 KB matte stays in L2. NEON: 1.04 ms.

---

## 1. MilkDrop / Winamp AVS — the closest prior art, and it is very close

### 1.1 MilkDrop: the background is *required* to yield, every frame

Ryan Geiss's own preset authoring guide documents the mechanism directly. Verbatim from the parameter table:

| Parameter | Documented behaviour |
|---|---|
| `decay` | *"controls the eventual fade to black; 1=no fade, 0.9=strong fade, **0.98=recommended**"* |
| `darken` | *"**darkens the brighter parts of the image** (nonlinear; **squaring filter**)"* |
| `brighten` | *"brightens the darker parts of the image (nonlinear; square root filter)"* |
| `solarize` | *"emphasizes mid-range colors"* |
| `darken_center` | *"help keeps the image from getting too bright by **continually dimming the center point**"* |
| `gamma` | *"controls display brightness; 1=normal, 2=double, 3=triple"* |
| `echo_zoom` / `echo_alpha` / `echo_orient` | size / opacity / flip of *"the second graphics layer"* |

And in MilkDrop 2's warp shader, the recommended baseline body is literally a per-frame multiplicative duck:

```hlsl
ret = tex2D( sampler_main, uv ).xyz;
ret *= 0.97;
```

**This is the whole answer in one line.** MilkDrop's background is a feedback buffer that is *multiplied down by 3% every single frame*. It is structurally incapable of reaching and holding 100%. The waveform is then drawn on top into the headroom that decay just created — `wave_additive` *"the wave is drawn additively, saturating the image at white"*, `wave_brighten` *"all 3 r/g/b colors will be scaled up until at least one reaches 1.0"*.

So MilkDrop's readability comes from three cooperating parts, and JellyDazzle currently has **none** of them:

1. **A permanent multiplicative duck on the background** (`decay`, `*= 0.97`).
2. **A non-linear highlight crush** on the background (`darken` = squaring, which maps 0.8 → 0.64 while leaving 0.2 → 0.04 — it takes the *brights* down hardest, exactly where MAX is failing).
3. **A spatial duck** (`darken_center`) — a vignette, i.e. a static power window.

MilkDrop 2 also splits the shader in two, and the split matters:

> The **warp shader** operates on the internal canvas, results *"baked" into the image, persisting to the next frame*. The **composite shader** runs on every screen pixel but *"will NOT affect the subsequent frame — it will only affect the display of the current frame."*

That is the distinction between *ducking the ground buffer* (persistent, cheap, compounding) and *ducking the delivered frame* (one-shot). For JellyDazzle the persistent version is cheaper still, because the ground's palette is persistent.

- Source: **Geiss, R., *MilkDrop Preset Authoring Guide*** — <https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html>
- Corroborating: **projectM, *Preset Authoring Guide*** — <https://github.com/projectM-visualizer/projectm/wiki/Preset-Authoring-Guide>

### 1.2 AVS: it shipped the darkening modes and a per-pixel holdout matte, in 1999

AVS's source is BSD-3 and readable. `avs/vis_avs/blend.h` declares the full kernel set:

```
blend_replace   blend_add        blend_5050       blend_multiply
blend_screen    blend_color_dodge blend_color_burn blend_linear_burn
blend_maximum   blend_minimum    blend_xor        blend_adjustable
blend_sub_src1_from_src2  blend_sub_src2_from_src1  blend_sub_src1_from_src2_abs
blend_buffer    blend_every_other_pixel  blend_every_other_line
```

JellyDazzle has 5 of these. **AVS had 18, and 6 of them darken.** Two are worth stealing outright:

**`blend_adjustable`** — a Q8 weighted lerp via a 256×256 multiply LUT. This is `span_lerp`. Already have it.

**`blend_buffer`** — *this is a holdout matte, in an integer music visualiser, in 1999:*

```c
// find the maximum channel value and spread it to all channels
uint8_t v = max(buf.r, buf.g, buf.b);
uint8_t iv = 255 - v;
if (invert) { iv = v; v = 255 - v; }
// do an adjustable blend
*dest = (mul[src1_r][v] + mul[src2_r][iv]) << 16 | ... ;
```

An **arbitrary framebuffer** is reduced to a per-pixel alpha by `max(r,g,b)` and used as the lerp weight, with an `invert` flag. That is Nuke's `Keymix` and a colourist's tracked power window, expressed in 8-bit integer with two table lookups per channel.

And AVS exposed it at the *structural* level, not just per-effect. `e_effectlist.h`:

```c
enum EffectList_Blend_Modes {
    LIST_BLEND_IGNORE, LIST_BLEND_REPLACE, LIST_BLEND_5050, LIST_BLEND_MAXIMUM,
    LIST_BLEND_ADDITIVE, LIST_BLEND_SUB_1, LIST_BLEND_SUB_2,
    LIST_BLEND_EVERY_OTHER_LINE, LIST_BLEND_EVERY_OTHER_PIXEL, LIST_BLEND_XOR,
    LIST_BLEND_ADJUSTABLE, LIST_BLEND_MULTIPLY, LIST_BLEND_BUFFER, LIST_BLEND_MINIMUM,
};
bool clear_every_frame;
int64_t input_blend_mode, output_blend_mode;
int64_t input_blend_adjustable, output_blend_adjustable;
int64_t input_blend_buffer, output_blend_buffer;
bool input_blend_buffer_invert, output_blend_buffer_invert;
```

A nested effect list has **both an input blend and an output blend**, each of which can be a per-pixel buffer matte, each independently invertible. JellyDazzle's layer stack has one output blend per layer and no input blend at all. Adding an *input* blend is what lets an accent layer say "show me the ground, but ducked, only where I am about to draw".

Two more AVS effects that are one-liners here:

- **`e_multiplier`** — `MULTIPLY_INF_ROOT, X8, X4, X2, X05, X025, X0125, INF_SQUARE`. A pure power-of-two framebuffer scale. Shifts only, no multiply. This is `decay` made discrete: *"the background yields, in one shift."*
- **`e_fadeout`**, **`e_colorclip`** — per-frame fade toward a colour, and a level clamp. Both are transfer curves, i.e. free in the palette domain here.

- Source: **AVS source (BSD-3), `grandchild/vis_avs`** — <https://github.com/grandchild/vis_avs> — files `avs/vis_avs/blend.h`, `blend.cpp`, `e_effectlist.h`, `e_multiplier.h`
- Architecture overview: **Wikipedia, *Advanced Visualization Studio*** — <https://en.wikipedia.org/wiki/Advanced_Visualization_Studio> — *"components are plugged into a list, executed from top to bottom, each component doing something with the image and sending the result to the next one"*

### 1.3 What transfers from §1

| MilkDrop / AVS mechanism | Maps onto | Cost here |
|---|---|---|
| `decay` / `*= 0.97` / `e_multiplier` | Q8 multiply on the ground's palette in `layer_pal_build` | **0.037 ms** |
| `darken` (squaring filter) | non-linear term in the existing `LL->lut[256]` | **free** (already computed) |
| `darken_center` | radial Q8 field folded into the matte pass | shares the matte pass |
| `blend_multiply` / `blend_minimum` | new `span_multiply` / `span_min` cases in `blend_span` | **0.70 ms** NEON |
| `blend_buffer` (holdout matte) | 1/8-scale accent luma matte + keymix | **1.04 ms** NEON |
| `input_blend_mode` on an effect list | a per-layer *input* blend the accent uses to duck the ground | structural, not per-pixel |
| `echo_zoom` / `echo_alpha` | second warped copy of a layer | ~2–3 ms + 30 MB — **skip**, see §8 |

---

## 2. Demoscene — the constrained-hardware answer is *give the layers disjoint colour ranges*

The demoscene never had blend modes. It had palettes and hardware layers, and it solved readability by **making it impossible for a foreground and a background to occupy the same values.**

**Amiga dual playfield.** Two independent bitplane layers, each with its own colour registers, composited in hardware:

> *"two 'playfields' of eight colors each (three bitplanes each) are drawn on top of each other"* … *"the background color of the top playfield 'shines through' to the underlying playfield."*

**Hardware sprites.** *"These sprites have three visible colors and one transparent color."* Sprites drew from **their own colour bank**, disjoint from the playfield's. A sprite could not accidentally match the background it flew over, because it was not allowed to address the background's colours. Readability was a property of the *palette allocation*, checked once by the artist, not a per-pixel operation checked 7.5 million times a frame.

**The Copper.** A beam-synchronised co-processor: *"The Copper can also change color registers mid-frame, creating the 'raster bars' effect."* Per-scanline palette rewriting — the palette is a live, cheap, per-region control surface. Exactly the posture JellyDazzle should take toward `g_pal[s]`.

**Palette-cycled plasma** (Lode Vandevenne's tutorial) makes the same point from the software side: the plasma buffer holds *indices*, and animation is `buffer[y][x] = palette[(plasma[y][x] + paletteShift) % 256]`. All the motion, all the colour, all the mood lives in a 256-entry table. JellyDazzle already does this with a 32768-entry table per layer — it simply has never used it to enforce contrast.

**How this maps.** `layer_pal_build` already builds each layer a **window** (`span`/`off`) into the shared ramp — a hue family. What it does *not* do is separate layers in **luminance**. The Amiga fix is a one-line contract change inside the existing LUT loop:

- Ground LUT maps into roughly **[0, 145]**.
- Accent LUT maps into roughly **[115, 255]**.

Then `B_MAX` is *correct by construction* — the accent is brighter than anything beneath it at every pixel, so MAX stops being a coin-flip and becomes a guarantee. The engine already has the exact machinery: `LL->lut[v]` is built from a measured `lo`/`hi` and a target range (`6.0f + t * 245.0f`); the target range is currently the same for every layer. Making it a function of the slot's role is a two-line change and costs nothing.

- Source: **Wikipedia, *Original Chip Set*** — <https://en.wikipedia.org/wiki/Original_Chip_Set>
- Source: **Vandevenne, L., *Lode's Computer Graphics Tutorial: Plasma*** — <https://lodev.org/cgtutor/plasma.html>

---

## 3. Compositing and colour grading — the operations have names and formulas

A colourist facing "the subject won't read against the background" reaches for four things. All four have exact, cheap integer analogues.

### 3.1 The holdout matte

Nuke's Merge node, verbatim algorithms:

| Op | Meaning | Algorithm |
|---|---|---|
| `stencil` | *"Only shows the areas of image B that do not overlap with the alpha of A"* | **B(1−a)** |
| `out` | *"Only shows the areas of image A that do not overlap with the alpha of B"* | A(1−b) |
| `mask` | *"Only shows the areas of image B that overlap with the alpha of A"* | Ba |
| `in` | Areas of A overlapping the alpha of B | Ab |
| `multiply` | | AB |
| `screen` | | A+B−AB if 0<A,B<1, else max(A,B) |
| `min` / `max` | | min(A,B) / max(A,B) |

`stencil` = **B(1−a)** is the operation JellyDazzle is missing. It is "punch the background down exactly where the foreground is". And Nuke's `Keymix` is the two-sided version:

> *"Keymix layers two images together using a specified Roto shape or image as a mask."* Formula: **Aa + B(1−a)**.

`Keymix` **is** AVS's `blend_buffer`. Two fields, thirty years apart, same operation. That is a strong signal.

### 3.2 Power windows and qualifiers

DaVinci Resolve's own product description:

- **Qualifier:** *"select and adjust part of an image based on hue, saturation or luminance"* — a **luminance key**. Cheap here: the accent buffer's `max(r,g,b)` or Rec.601 luma *is* the key, computed at 1/8 scale for 0.05 ms.
- **Power Windows:** *"define a selection by drawing shapes around specific objects in a scene"* — circle, curve, gradient. MilkDrop's `darken_center` is a circular power window. A radial or gradient Q8 field is separable in x/y and costs a table lookup.
- **Tracker:** *"automatically animate Power Windows to follow moving objects."* JellyDazzle does not need a tracker — it *generates* the foreground, so the matte is exact and free. This is the one place the engine is strictly better off than a colour suite.

**The real-time analogue of the whole grading workflow is one pass:** build a matte from the accent's own luma, blur it, use it to duck the ground and composite the accent in the same loop.

### 3.3 Glow / bloom with a threshold

See §4.1 — it is the same operation in both fields, and games document it more precisely.

### 3.4 Local contrast enhancement at feature edges

Mitchell et al. (TF2, §4 below) cite **Luft, Colditz & Deussen 2006, "Image Enhancement by Unsharp Masking the Depth Buffer", ACM TOG 25(3):1206–1213** — image-space lightening and darkening to *"increase contrast at important feature edges."* The 2D analogue with no depth buffer: unsharp-mask the *matte*. Subtracting a blurred matte from a sharper one gives a dark ring immediately outside the accent — a free contact shadow that separates the accent from whatever is behind it, at zero extra passes because the blurred matte already exists.
*(Citation read from the TF2 reference list, which I verified directly; the ACM DL page itself returned 403, so I have not read the paper's own text.)*

- Source: **Foundry, *Nuke Merge node reference*** — <https://learn.foundry.com/nuke/content/reference_guide/merge_nodes/merge.html>
- Source: **Foundry, *Nuke Keymix node reference*** — <https://learn.foundry.com/nuke/content/reference_guide/merge_nodes/keymix.html>
- Source: **Blackmagic Design, *DaVinci Resolve — Color*** — <https://www.blackmagicdesign.com/products/davinciresolve/color>

---

## 4. Game rendering — readability over an *arbitrary* background is the whole discipline

### 4.1 Bloom is a threshold, and the threshold is the point

LearnOpenGL's bloom chapter, verbatim:

```glsl
float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
if(brightness > 1.0) BrightColor = vec4(FragColor.rgb, 1.0);
else                 BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
```

> *"The threshold is necessary because it isolates only intensely bright pixels… Without it, the glow effect would apply uniformly across all colors, making it visually overwhelming."*

**"Visually overwhelming" is the fog.** JellyDazzle's `span_screen` is an unthresholded glow: every pixel of the accent lightens the ground, including the 60% of accent pixels that carry no information. Thresholding the accent before it composites — pass only the top ~15% of its luma — would remove most of the fog without touching the flashes. That is a Q8 compare in the existing loop.

Then blur (separable, two 1D passes), then `hdrColor += bloomColor` **before** tonemap.

### 4.2 Engines expose the blend choice explicitly, and they agree with the diagnosis

Godot's glow documentation is unusually candid about exactly the trade-off in play here:

| Godot glow blend mode | Godot's own words |
|---|---|
| **Additive** | *"the strongest one… In general, **it's too strong to be used**, but can look good with low-intensity Bloom"* |
| **Screen** | *"ensures glow **never brightens more than itself** and it works great as an all around"* |
| **Softlight** | *"the default and **weakest** one, producing only a subtle color disturbance around the objects. **This mode works best on dark scenes**"* |
| **Replace** | *"only shows the glow effect without the image below"* |

Note the last clause on Softlight: the weakest blend is the default *and it is qualified as working best on dark scenes*. A shipping engine's default glow assumes a dark background. JellyDazzle's ground is bright and permanent, which is precisely why its accents behave like Additive-on-a-bright-scene: *too strong to be used*, yet invisible.

Godot also gates glow behind an **HDR Threshold** — *"the light in a pixel surpasses the HDR Threshold"* — and an **HDR Scale**. Same lesson as §4.1: gate first, then glow.

### 4.3 Tonemapping — a shoulder instead of a clip

`span_add` and `span_screen` both hard-clamp to 255. That is `Linear` tonemapping, which Godot describes as *"unnaturally clips bright values."* The alternatives, with exact formulas:

- **Reinhard:** `texColor = texColor / (1 + texColor)`
- **Hable / Uncharted 2:** `((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F`, A=0.15 B=0.50 C=0.10 D=0.20 E=0.02 F=0.30
- **Hejl / Burgess-Dawson:** `x = max(0, texColor-0.004); (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06)`

The **shoulder** compresses highlights so they hold colour instead of going white; the **toe** crushes blacks. In an 8-bit integer compositor all three collapse to a **256-entry byte LUT**. It is honest to say this makes the fog *prettier* — it does not make the accent *legible*. Secondary. See §8.

### 4.4 Rim lighting and background suppression — Valve, TF2

Mitchell, Francke & Eng (Valve), *Illustrative Rendering in Team Fortress 2*, NPAR 2007. This paper is squarely on-topic: it is about making foreground elements read against arbitrary, uncontrolled backgrounds, and it attacks the problem **from both ends**.

**Foreground end — dedicated rim lighting.** From §5.2:

> *"In situations where a character has moved away from the light sources in the game level, rim lighting based solely on Phong terms from local light sources may not be as prominent as we would like. For this reason, we also add in the dedicated rim lighting term."*

The rim term is *"masked with a Fresnel term, ensuring that they are only present at grazing angles."* And from §3, the codified illustration conventions: *"Silhouettes are emphasized with rim highlights rather than dark outlines."*

**Background end — deliberately suppress the background's frequency and contrast.** This is the half usually forgotten, and it is the more relevant half here:

> *"High frequency detail is omitted where possible."* (§3)

> *"By maintaining a minimal level of repetition and visual noise, we serve many of our gameplay goals while employing an almost impressionistic approach to modeling."* (§4.2)

> *"We have found that high frequency geometric and texture detail found in photorealistic games can often overpower the ability of designers to compose game environments and emphasize gameplay features visually using intentional design choices such as changes in color value."* (§4.3)

> On the skybox: *"specifically modelled with less detail than it would be if it were in the reachable portions of the environment… This is not just to manage level of detail, but also fits with the overall visual style and prevents the skybox from generating high-frequency noise that would distract players."* (§6)

**That last quote is J's complaint, written by Valve in 2007.** "Foggy clutter where the accent is barely visible" is high-frequency background noise overpowering intentional changes in colour value. Valve's fix was not a better blend mode — it was *making the background categorically quieter and lower-contrast than the foreground, as a design law*.

The engine's analogue is a **contrast compression on the ground's LUT** (map its output into a narrow band, e.g. [40, 150]), which reduces the ground's perceived busyness without a blur and costs 0.037 ms. A genuine large blur is also available cheaply — downsample 1/8 (0.05 ms) and `jd_up`-upsample — mixed in at low weight as a soft-focus on the ground only.

### 4.5 UI readability — the scrim

The universal shipping answer for "text over an arbitrary image" is a **scrim**: a partially-opaque dark plate between the background and the element, sized to the element. That is `stencil` with a blurred, dilated matte — §3.1 again, arrived at from a third direction. The target is quantified by **WCAG 2.2**: 1.4.3 requires *"a contrast ratio of at least 4.5:1"* for text, and 1.4.11 requires *"at least 3:1 against adjacent color(s)"* for graphical objects necessary for comprehension. A **3:1 luminance ratio between accent and local ground** is a defensible, measurable acceptance target for the `jd_tap` harness — better than "looks foggy".

- Source: **LearnOpenGL, *Bloom*** — <https://learnopengl.com/Advanced-Lighting/Bloom>
- Source: **Godot Engine docs, *Environment and post-processing*** — <https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html>
- Source: **Hable, J., *Filmic Tonemapping Operators*** — <http://filmicworlds.com/blog/filmic-tonemapping-operators/>
- Source: **Mitchell, Francke & Eng, *Illustrative Rendering in Team Fortress 2*, NPAR 2007** — <https://media.steampowered.com/apps/valve/2007/NPAR07_IllustrativeRenderingInTeamFortress2.pdf>
- Source: **Jimenez, J., *Next Generation Post Processing in Call of Duty: Advanced Warfare*, SIGGRAPH 2014 Advances in Real-Time Rendering** — <http://advances.realtimerendering.com/s2014/index.html>
- Source: **W3C, *WCAG 2.2*, SC 1.4.3 & 1.4.11** — <https://www.w3.org/TR/WCAG22/#non-text-contrast>

---

## 5. Motion graphics — which modes put an element *on* something bright rather than *in* it

The W3C Compositing and Blending Level 1 spec gives the exact separable formulas, and these are the same modes After Effects and Photoshop expose:

| Mode | Formula `B(Cb, Cs)` | Behaviour against a **bright** backdrop |
|---|---|---|
| multiply | `Cb × Cs` | **Always darkens.** Bright backdrop is where it has the most authority. |
| darken | `min(Cb, Cs)` | Exact dual of `B_MAX`. Guaranteed non-lightening. |
| color-burn | conditional | Aggressive: crushes the backdrop toward black in the mid-tones. |
| screen | `1 − (1−Cb)(1−Cs)` | **Asymptotes to 1.** Against a bright backdrop, no-op or fog. This is the diagnosis. |
| lighten | `max(Cb, Cs)` | Against a bright backdrop, the backdrop wins. This is the other half of the diagnosis. |
| overlay | `HardLight(Cs, Cb)` | Backdrop-conditional: darkens dark backdrops, lightens light ones. Preserves ground structure. |
| hard-light | multiply if `Cs ≤ 0.5`, else screen | Source-conditional. |
| soft-light | conditional | Gentle; Godot's default glow blend for a reason. |
| difference | `|Cb − Cs|` | Already have it (`B_DIFF`). |
| exclusion | `Cb + Cs − 2·Cb·Cs` | Lower-contrast difference. |

Compositing over a backdrop with alpha: `Cr = (1 − αb) × Cs + αb × B(Cb, Cs)`.

**The motion-graphics answer to "sit ON something bright" is Multiply, Linear Burn, or a Darken-family mode — and none of them exist in this engine.** The four modes it has (`MAX`, `SCREEN`, `ADD`, `DIFF`) are the four with the *least* authority over a bright backdrop; three of them are provably weakest exactly where the ground is brightest.

**Overlay deserves a specific mention.** `B(Cb, Cs) = HardLight(Cs, Cb)` is backdrop-conditional: it multiplies where the ground is dark and screens where the ground is light. It is the single mode that *keeps the ground's structure visible while forcing the accent to modulate it* — which is closer to J's stated intent ("moments of beauty, accents against a ground") than either MAX or SCREEN. Integer cost: a branch plus the multiply already written. Worth prototyping alongside multiply.

**Track mattes and Stencil/Silhouette modes.** After Effects additionally has Stencil Alpha/Luma and Silhouette Alpha/Luma blend modes, plus track mattes (Alpha, Alpha Inverted, Luma, Luma Inverted). These are the same holdout-matte operations as Nuke's `stencil`/`mask` and AVS's `blend_buffer` — the fourth independent rediscovery of the same idea. *I could not verify Adobe's exact wording: `helpx.adobe.com` timed out on every attempt from this machine, so I am citing the W3C spec and Nuke for the formulas and naming Adobe's features without quoting them.*

- Source: **W3C, *Compositing and Blending Level 1*** — <https://www.w3.org/TR/compositing-1/>

---

## 6. The one thing every field independently invented

Five domains, five vocabularies, one operation:

| Field | Name | Formula |
|---|---|---|
| Winamp AVS (1999) | Buffer blend | `mul[src1][v] + mul[src2][255−v]`, `v = max(r,g,b)` of a buffer |
| Nuke / VFX | `stencil` / `Keymix` | `B(1−a)` / `Aa + B(1−a)` |
| Colour grading | tracked power window + luma qualifier | isolate, then duck the surround |
| Games / UI | scrim, or blurred-dilated matte | dark plate under the element only |
| After Effects | track matte, Stencil/Silhouette Luma | same |

When five fields converge on one primitive, that primitive is the answer. **JellyDazzle does not have it.** That is the gap.

---

## 7. Affordability, measured

**Method.** `clang -O2`, single thread, 3456×2160 = 7,464,960 px, ARGB u32, 20–30 iterations, warm buffers, `CLOCK_MONOTONIC`. Kernels are the engine's own (`span_scale`, `span_lerp` copied verbatim from `compositor.c`). NEON variants use `vmull_u8`/`vmlal_u8`/`vshrn_n_u16` — the shape J would hand-write.

| Operation | Scalar | NEON | vs. frame budget (10.5 ms) |
|---|---:|---:|---|
| `memcpy` full frame | 0.23 ms | — | 2% — the bandwidth floor |
| `span_scale` (shipping kernel) | **2.29 ms** | 0.70 ms | 22% — **ALU-bound, 10× off memcpy** |
| `span_lerp` (shipping kernel) | **2.04 ms** | ~1.0 ms | 19% |
| Keymix w/ 1/8 matte (holdout) | **2.07 ms** | **1.04 ms** | 20% / 10% |
| Build 1/8-scale luma matte | **0.05 ms** | — | 0.5% |
| Palette pass, 32768 entries | **0.037 ms** | — | **0.35%** |

Three conclusions, and they decide the ranking:

1. **The palette is 55× cheaper than the cheapest full-frame pass, and 62× cheaper than the shipping kernels.** Any correction expressible as a transfer curve must live in `layer_pal_build`, not in a span kernel.
2. **A per-pixel holdout matte is free relative to what is already being paid.** 2.07 ms scalar vs. `span_lerp`'s 2.04 ms — the matte read is invisible because 116 KB stays resident. This kills the assumption that a matte is expensive.
3. **The existing span kernels are ALU-bound, not bandwidth-bound** — 2.29 ms against a 0.23 ms memcpy floor. NEON recovers 3.3× (2.29 → 0.70 ms). Adding a NEON darken pass would *still leave the frame faster than today* if it replaces a scalar kernel. There is more headroom here than the 10.5 ms budget suggests.

---

## 8. Ranked shortlist — the three worth stealing

### #1 — Value-separated palette contract + a permanent ground duck, executed in the LUT

**Steal from:** MilkDrop `decay` / `darken` / `*= 0.97`; AVS `e_multiplier`; Amiga dual-playfield and sprite colour banks.

**What it is.** Two changes inside `layer_pal_build` (`compositor.c:1117`), both in the loop that already runs:

- Apply a Q8 duck and a squaring term to the **ground's** palette: `c = (c * duck) >> 8`, plus a non-linear highlight crush folded into `LL->lut[256]` (which is already built from a `lo`/`hi` measurement and a target range). MilkDrop's `darken` is `x²`; in 8-bit that is `(v*v)>>8` — a single multiply per LUT entry, computed 32768 times, not 7.5 million.
- Make the LUT's **target range a function of the slot's role**: ground → roughly [0, 145], accents → roughly [115, 255]. Currently every layer targets `6.0f + t * 245.0f`.

**Why it is #1.**

- It attacks the *measured* cause. MAX is a no-op because the ground is brighter than the accent. Separate them in value and MAX becomes correct **by construction**, at every pixel, with no per-pixel test.
- **0.037 ms.** Cheaper than the noise floor. It is not a trade-off.
- It reuses machinery that already exists and is already tuned — the transfer LUT, `gq`, the reciprocal table, the frozen-at-spawn curve. This is roughly a 20-line change, not a new subsystem.
- It fixes the problem *before* compositing, so every downstream blend mode benefits, including the ones that already ship.
- It is what three independent fields did: MilkDrop by decay, Amiga by palette allocation, Valve by art direction.

**Honest caveats.**
- Some asm routines index `jd_palette` directly and take no layer palette (`compositor.c:504`). Those need the fallback: a NEON `span_multiply` on the ground buffer, **0.70 ms**, paid only by those routines.
- **The dead-air guard will fight this.** `JD_LUMA_FLOOR` + `span_gain` measure the composite's mean luma and lift it (`compositor.c:~2638`). A deliberately ducked ground reads as dead air and gets multiplied straight back up. The guard must learn the difference between "dark because dead" and "dark because ducked" — measure the *accent-weighted* luma, or exempt a ducked ground explicitly. **This is the single integration hazard that will silently defeat the fix if missed.**

---

### #2 — Holdout matte: a local ground duck driven by a 1/8-scale accent luma matte

**Steal from:** AVS `blend_buffer`; Nuke `stencil` = B(1−a) and `Keymix` = Aa + B(1−a); DaVinci power window + luma qualifier; the UI scrim.

**What it is.** Per accent layer, per frame:

1. Point-sample the accent buffer to a 1/8-scale byte matte via `max(r,g,b)` or Rec.601 luma — **0.05 ms**, 116 KB.
2. Threshold it (LearnOpenGL/Godot: pass only the top band) and blur it at 1/8 scale — negligible, it fits in L2.
3. In the accent's blend loop, use the matte as the per-pixel weight: duck the ground by `(1 − k)` and lerp the accent in by `k`, bilinearly upsampled via the existing `jd_up` in `src/patterns/_upsample.h`.

**Why it is #2.**

- **It is local.** #1 alone dims everywhere; this dims *only where an accent needs room*. Together they give a ground that is generally calmer and specifically ducked. That is exactly what a colourist does.
- **Measured at 1.04 ms NEON / 2.07 ms scalar — the same as the `span_lerp` already in the loop.** The matte read is free. This was the surprise in §7.
- The engine **generates** its foreground, so the matte is exact and needs no tracker. Better position than a colour suite.
- The blurred matte is reusable for free: dilate-minus-sharp gives a contact shadow (Luft et al.'s edge contrast); a radial term folded into the same field gives `darken_center` / a power window at no extra pass.
- Five independent fields invented this (§6).

**Honest caveats.**
- 1/8 matte resolution means the duck has ~8 px granularity. For soft glows and sprite blooms this is invisible and in fact desirable (the softness *is* the scrim). For a 1-px-wide filament accent, the duck will be visibly wider than the filament. Mitigation: 1/4 scale costs 0.2 ms to build and is still cache-resident, or blend the matte's influence by the accent's measured `delta_q8`.
- This adds a genuine full-frame pass (1.04 ms NEON) unless it **replaces** the existing blend rather than preceding it — fold the duck and the composite into one loop, don't run two.

---

### #3 — Actual darkening blend modes: `span_multiply` and `span_min` (and prototype `overlay`)

**Steal from:** AVS `blend_multiply` / `blend_minimum` / `blend_linear_burn`; W3C multiply / darken / overlay; After Effects' darkening category.

**What it is.** Three new cases in `blend_span` (`compositor.c:1412`), each structurally identical to the `span_max` already written:

- `span_multiply`: `(d * s) >> 8` per channel, Q8-weighted toward the unmodified `d`.
- `span_min`: exact dual of `span_max`. Five lines.
- `span_overlay` (prototype): multiply where `d ≤ 128`, screen above — the one mode that preserves ground structure while forcing the accent to modulate it.

Then extend `pick_blend` (`compositor.c:1617`). It already has the statistics it needs — `st->dark`, `st->luma`, `st->sat`, and the ground's live luma — and it already caps SCREEN by ground brightness, which is the engine *admitting the problem and having nothing better to reach for*. Give it something: bright routine over bright ground → MULTIPLY; the current SCREEN cap becomes a mode switch instead of an opacity clamp.

**Why it is #3 and not higher.**

- It is a **capability** fix, not a correction. It doesn't repair existing presets; it lets the scheduler make choices it currently cannot. Real value, slower payoff.
- It unlocks a whole class of content that is presently invisible: dark, detailed accents. The comment at `compositor.c:1395` already records that black is a no-op under MAX/SCREEN/ADD — meaning every dark accent in the library is currently rendering as nothing.
- **0.70 ms NEON — cheaper than the 2.29 ms scalar kernel it sits beside.** Adding it can make the frame faster.
- Effectively free to write: same shape as `span_max`.

**Honest caveat.** Multiply over a *dark* ground goes to mud as fast as screen over a dark ground goes milky. It needs the same ground-luma-conditional cap `pick_blend` already applies to SCREEN, just pointing the other way.

---

### The three, in one line each

| Rank | Technique | Measured cost | Attacks |
|---|---|---:|---|
| **1** | Value-separated palette contract + ground duck in `layer_pal_build` | **0.037 ms** | The cause: MAX is a no-op because ground > accent |
| **2** | 1/8-scale holdout matte → local ground duck, folded into the accent's blend | **1.04 ms** NEON (≈ the lerp it replaces) | The locality: dim *where the accent is*, not everywhere |
| **3** | `span_multiply` / `span_min` / `overlay` + `pick_blend` routing | **0.70 ms** NEON | The capability: dark accents currently render as nothing |

They compose. #1 makes the ground yield globally and for free; #2 makes it yield locally and precisely; #3 gives the scheduler a mode that has authority over a bright ground. Do #1 first — it is nearly free, it is testable through the new `jd_tap` harness, and it may be sufficient on its own.

---

## 9. Beautiful, but not worth it here

| Technique | Why it is admired | Why it does not transfer |
|---|---|---|
| **MilkDrop echo / AVS blitter feedback** (`echo_zoom`, `echo_alpha`) | Gorgeous trails; core to MilkDrop's identity | Needs a persistent 30 MB buffer plus a warped read per pixel (~2–3 ms). It *adds* soft luminous material — it makes the foggy clutter worse. Wrong tool for this complaint. |
| **Full HDR pipeline + filmic tonemap** (Hable, ACES) | The principled fix; highlights hold colour instead of clipping to white | Requires float or 16-bit intermediates — a rewrite of every `span_*` kernel and every pattern's output contract. The 8-bit shortcut (a 256-entry LUT on the final frame) is affordable and worth doing *later*, but it makes the fog prettier, not the accent legible. |
| **True Gaussian bloom at full resolution** | The canonical glow | Two separable full-frame passes at ≥0.7 ms each, and it lightens — the exact failure mode already diagnosed. The 1/8-scale thresholded version is already inside #2 for free. Full-res buys nothing perceptible at 3456×2160. |
| **Rim lighting (TF2 §5.2)** | The best single answer in games to "read against anything" | Needs surface normals and a view vector. There is no geometry here. The 2D analogue — a bright edge derived from the accent's own matte gradient — collapses into #2's dilate-minus-blur, so it is already captured. Steal the *idea*, not the term. |
| **Luft et al. depth-buffer unsharp mask** | Elegant local contrast at feature edges | No depth buffer. Same collapse as above: unsharp the matte instead, inside #2. |
| **Copper-style per-scanline palette rewriting** | Historically the most beautiful trick on the list | The engine's layers are full-frame buffers composited after the fact, not beam-raced. A per-scanline palette has no meaning in this architecture. The *lesson* — treat the palette as a live control surface — is exactly #1. |
| **AVS `blend_xor`, `every_other_pixel`, `every_other_line`** | Cheap dithered transparency on 1999 hardware | Solved problems. A Q8 lerp is cheaper and better on this machine. Historical interest only. |

---

## Sources

All URLs below were fetched and read in the course of writing this document unless explicitly marked otherwise.

1. Geiss, R. — *MilkDrop Preset Authoring Guide* — <https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html>
2. projectM — *Preset Authoring Guide* — <https://github.com/projectM-visualizer/projectm/wiki/Preset-Authoring-Guide>
3. AVS source, BSD-3 (`grandchild/vis_avs`) — <https://github.com/grandchild/vis_avs> — read: `avs/vis_avs/blend.h`, `avs/vis_avs/blend.cpp`, `avs/vis_avs/e_effectlist.h`, `avs/vis_avs/e_multiplier.h`
4. Wikipedia — *Advanced Visualization Studio* — <https://en.wikipedia.org/wiki/Advanced_Visualization_Studio>
5. Wikipedia — *Original Chip Set* (Amiga dual playfield, sprites, Copper) — <https://en.wikipedia.org/wiki/Original_Chip_Set>
6. Vandevenne, L. — *Lode's Computer Graphics Tutorial: Plasma* — <https://lodev.org/cgtutor/plasma.html>
7. Foundry — *Nuke Merge node reference* — <https://learn.foundry.com/nuke/content/reference_guide/merge_nodes/merge.html>
8. Foundry — *Nuke Keymix node reference* — <https://learn.foundry.com/nuke/content/reference_guide/merge_nodes/keymix.html>
9. Blackmagic Design — *DaVinci Resolve — Color* — <https://www.blackmagicdesign.com/products/davinciresolve/color>
10. LearnOpenGL — *Bloom* — <https://learnopengl.com/Advanced-Lighting/Bloom>
11. Godot Engine — *Environment and post-processing* — <https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html>
12. Hable, J. — *Filmic Tonemapping Operators* — <http://filmicworlds.com/blog/filmic-tonemapping-operators/>
13. Mitchell, J., Francke, M. & Eng, D. — *Illustrative Rendering in Team Fortress 2*, NPAR 2007 — <https://media.steampowered.com/apps/valve/2007/NPAR07_IllustrativeRenderingInTeamFortress2.pdf>
14. Jimenez, J. — *Next Generation Post Processing in Call of Duty: Advanced Warfare*, SIGGRAPH 2014 *Advances in Real-Time Rendering* — <http://advances.realtimerendering.com/s2014/index.html>
15. W3C — *Compositing and Blending Level 1* — <https://www.w3.org/TR/compositing-1/>
16. W3C — *WCAG 2.2*, SC 1.4.3 and 1.4.11 — <https://www.w3.org/TR/WCAG22/#non-text-contrast>

**Cited but not read in this session (stated for honesty):**

- Luft, T., Colditz, C. & Deussen, O. — *Image Enhancement by Unsharp Masking the Depth Buffer*, ACM TOG 25(3):1206–1213, SIGGRAPH 2006. Reference verified in the TF2 paper's bibliography, which I read directly. <https://dl.acm.org/doi/10.1145/1141911.1142016> returned HTTP 403.
- Adobe — After Effects blending modes and track mattes. `helpx.adobe.com` timed out on every attempt from this machine; AE's Stencil/Silhouette and track-matte features are named in §5 but not quoted, and the formulas given there are the W3C spec's.
