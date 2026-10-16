# Star Trek: New Worlds: textures (`.lbp`, `.mp0`–`.mp4`, `.eb0`, `.eb1`, `.fbp`)

Status: **understood for format 6**, confirmed visually on real textures.
Decoder: `src/engines/newworlds/texture.cpp`; the New Worlds engine currently
shows every texture in an install. `reluva-tool lbp-info <file> <prefix>`
decodes one to BMP files.
Stored in `.cat` archives with chunk tag `0x16`.

All values little-endian.

```
u32 format           6 in every texture checked so far
u32 palette count    256
palette              count × 4 bytes: blue, green, red, 0 (like a Windows RGBQUAD)
u32 0                (unknown, 0 so far)
u32 width
u32 height
layer 1              width × height bytes: palette indices
layer 2              width × height bytes: alpha (0 transparent, 255 opaque)
```

Evidence (from `rommisc.cat`, decoded with `lbp-info`):

| Texture | Header | Data bytes |
|---|---|---|
| `lbp/txr/rom/rom_hs.lbp` | 0, 64, 64 | 8192 = 2 × 64 × 64 |
| `lbp/txr/rom/rflare.lbp` | 0, 128, 128 | 32768 = 2 × 128 × 128 |
| `lbp/txr/rom/rimpact.lbp` | 0, 256, 256 | 131072 = 2 × 256 × 256 |
| `lbp/txr/rom/rb_muzzl.mp0` | 0, 64, 64 | 8192 = 2 × 64 × 64 |

Decoding `rom_hs` and `rb_muzzl` as 64 wide gave recognisable images (with the
second layer showing as a second image underneath), while 64-wide decodes of
the larger textures looked stretched, as expected if their real widths are 128
and 256. Every texture size seen in the archives fits 8 + 1024 + 12 + 2 × w × h
for power-of-two sizes.

Confirmed by viewing the decoded images:

- Decoded at the header size, `rom_hs`, `rflare` and `rb_muzzl` look right.
- `rimpact.lbp` (256 × 256) is a 4 × 4 grid of 64 × 64 frames, presumably an
  animated impact effect stored as a sprite sheet.
- Layer 2 looks like a transparency mask of the image.
- With the palette read as blue, green, red, `explode.lbp` is orange and
  yellow fire; read as red, green, blue it isn't.

## Open questions

- What the u32 0 before the width means.
- Whether other format values exist, e.g. in `.fbp` font textures or `.eb0`
  environment maps.
- How sprite sheets are divided into frames (fixed grid, or described elsewhere).

## Texture sets

Container tag `0x29` holds an `.mmb` (tag `0x2a`) followed by one or more
texture levels (`.mp0`, `.mp1`… or `.eb0`, `.eb1`). `rb_muzzl.mmb` is
`01 01 00 00` followed by the float 0.25; the second byte may be the number of
levels (`rb_muzzl` has only `.mp0`).
