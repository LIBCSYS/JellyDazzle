#!/usr/bin/env python3
"""Validate every class in the taxonomy is achievable: one reference palette each."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette_score import *

REF = {
 'mono_accent': (  # deep teal field, amber embers
   "Ironworks Teal",
   ["03080a","05161c","082935","0b3f4f","0d586d","10738c","1392b0","17b3d6",
    "33d0ee","61e0f5","8fecff","c2991d","6b4d0b"]),

 'duotone': (      # cobalt vs rust, two inks
   "Cobalt & Rust",
   ["05070f","0a0f22","111a3d","18265c","1f3480","2a47a8","3a5fd0","5a82e8",
    "8aabf2","c0d4fa","1a0705","3a1108","5e1e0a","8a2f0d","b84512","dc6320",
    "ee8c45","f7b47e","fdd9bd"]),

 'analogous': (    # green -> teal -> blue only
   "Fern Shallows",
   ["03100a","062015","0a3220","0f4a2c","14653a","1a8347","2aa05c","44bb76",
    "6ad096","0a2e2c","0f4744","146360","1a807c","28a09a","46bdb6","72d6d0",
    "0d2340","123356","18466f","20598a","2d70a6","4a8cc0"]),

 'split_complement': (  # gold dominant vs blue-violet + cyan-blue
   "Reliquary",
   ["060d08","0c1c11","12301b","185026","1e7233","26953f","35b84d","5ed76c",
    "93ea99","c9f7c5","2b0620","4d0a38","731055","a3167a","cc2b9e","e85fc0",
    "f79ad9","1a0a3a","2d1263","46209c","6435c9","8a5ce8","b394f2"]),

 'neon_on_black': (  # magenta/cyan lasers on void
   "Voltage",
   ["000000","010104","020208","03030d","050512","060618","08081f","0a0a26",
    "2a0033","55006b","8f00b0","c400e6","ff00ff","ff5cff","001f2b","003d55",
    "00708f","00a8c4","00e5ff","5cf4ff"]),

 'pastel_wash': (   # soft sky / blush / mint, nothing dark
   "Sugar Haze",
   ["fdf6f8","fbe9ef","f7d9e4","f2c9d8","eab8cd","f5efe0","efe4c9","e6d8b8",
    "dceac9","c9e0d4","b8d6d9","c4d4ea","d2ddf0","e0e8f6","eef2fb","ffffff",
    "f0d8d0","e4c8c4"]),

 'metallic': (      # gold specular curve
   "Struck Gold",
   ["050301","0d0803","1a1005","2a1a07","3d270a","54360c","6d470f","8a5c13",
    "a87318","c58d1f","dca62c","edbc45","f6d066","fbe295","fdf0c4","fffaea",
    # return leg: comes back down cool-grey so the CYCLIC wrap has no cliff
    "d6cbb2","a89e88","7a7263","4f4940","2b2721","141210"]),

 'stark': (         # black / white / one red slash
   "Newsprint",
   ["000000","000000","060606","0c0c0c","141414","1c1c1c","f4f4f4","fafafa",
    "ffffff","ffffff","f0f0f0","e6e6e6","d8d8d8","2a2a2a","cc0a14","ffffff",
    "000000","1a1a1a","ededed"]),

 'earth': (         # clay / moss / bark, muted
   "Kiln",
   ["2b211a","3a2c22","4a382a","5c4633","6e5540","806549","8f7457","9c8467",
    "a89478","4a4a2e","5a5c39","6a6e45","7a8052","8a9260","9aa470","3f3228",
    "554438","6b5748","816b58"]),

 'full_spectrum': ( # the whole wheel, earned
   "Prism Riot",
   ["0a0a0f","d0102a","e8541a","f59a10","f2d40c","9fd21a","2fb84a","16b58a",
    "12a0c4","1c6fd8","4a3fd0","8a2fc4","c81f96","f04a6a","ffffff","2a2a35"]),
}

if __name__ == '__main__':
    print(f'{"class":18s} {"reference":18s} fit  pass  offenders')
    print('-' * 92)
    allpass = True
    for cls, (name, cols) in REF.items():
        cols = order_ramp(cols)
        s, ok, det = score(cols, cls)
        bad = [f'{k}={v}' for k, (v, sub, t) in det.items() if sub < 1.0]
        allpass &= ok
        print(f'{cls:18s} {name:18s} {s:.2f} {"OK " if ok else "FAIL"}  {"; ".join(bad)}')

    print()
    print('--- pairwise diversity (min should exceed 0.16 cross-class) ---')
    sigs = {c: signature(order_ramp(v[1])) for c, v in REF.items()}
    ks = list(sigs)
    worst = (9, None, None)
    for i in range(len(ks)):
        for j in range(i + 1, len(ks)):
            d = distance(sigs[ks[i]], sigs[ks[j]])
            if d < worst[0]:
                worst = (d, ks[i], ks[j])
    print(f'closest pair: {worst[1]} <-> {worst[2]}  d={worst[0]:.3f}')
    print('ALL CLASSES ACHIEVABLE' if allpass else 'SOME CLASSES UNREACHABLE -- retune')
