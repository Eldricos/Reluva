# Star Trek: New Worlds: textures (`.lbp`, `.mp0`–`.mp4`, `.eb0`, `.eb1`, `.fbp`)

Status: **formats 1 and 6 understood** (1,559 + 479 of the 2,108 textures in
the retail archives); textures without a stored size are partly understood.
Decoder: `src/engines/newworlds/texture.cpp`; the New Worlds engine currently
shows every texture in an install. `reluva-tool lbp-info <file> <prefix>`
decodes one to BMP files.
Stored in `.cat` archives with chunk tag `0x16`.

All values little-endian.

```
u32 format           1 = one layer (opaque), 6 = two layers (with alpha)
u32 palette count    256
palette              count × 4 bytes: blue, green, red, 0 (like a Windows RGBQUAD)
u32 size flag        0 = width and height follow, 1 = they don't
[u32 width]          only if the flag is 0
[u32 height]         only if the flag is 0
layer 1              width × height bytes: palette indices
layer 2              format 6 only: width × height bytes of alpha (0 transparent, 255 opaque)
```

### Formats

`reluva-tool tex-survey` over all 23 retail archives (2,108 textures):

| Format | Size flag | Count | Used for |
|---|---|---|---|
| 1 | 0 | 1,559 | most textures: terrain and building `.lbp`, every mipmap level `.mp0`–`.mp4`, environment maps `.eb0/.eb1`, fonts `.fbp` |
| 6 | 0 | 479 | textures with transparency: effects, trees, bushes, some unit skins |
| 1 or 6 | 1 | ~70 | interface images (`lbp/gui/...`), sprites under `res/kurtscou/`, a few star and planet textures |

Mipmap sets (`.mp0`, `.mp1`… in a `0x29` container) halve in size each level,
each with its own palette: e.g. `lbp/txr/map/aln/brid01` is 256 × 256,
128 × 128, 64 × 64 (66580, 17428, 5140 bytes).

### Textures without a stored size

When the flag is 1, every file is exactly 4 bytes longer than palette +
pixel data: e.g. `lbp/gui/point.lbp` is 1304 = 8 + 1024 + 4 + 256 (16 × 16),
`lbp/txr/solsys/tabras01.mp0` is 132120 = 8 + 1024 + 4 + 2 × 256 × 256. So
the pixels follow the flag directly, and the size must come from elsewhere.
The decoder infers it (square, else 2:1) and marks it as a guess; non-square
ones such as `res/kurtscou/kscout1l.lbp` (3840 pixels) can't be decoded yet.

Many of these palettes have magenta (`fe 02 fe`) or green (`02 aa 02`) at
index 0, the classic colour-key colours, so index 0 may be transparent in
them. Not confirmed.

### Other

`misc.cat` stores one string table (`str/eng/frend.stb`) with the texture tag.

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

- Where the size of flag-1 textures is defined, and whether index 0 is transparent in them.
- Whether other format values exist, e.g. in `.fbp` font textures or `.eb0`
  environment maps.
- How sprite sheets are divided into frames (fixed grid, or described elsewhere).

## Texture sets

Container tag `0x29` holds an `.mmb` (tag `0x2a`) followed by one or more
texture levels (`.mp0`, `.mp1`… or `.eb0`, `.eb1`). `rb_muzzl.mmb` is
`01 01 00 00` followed by the float 0.25; the second byte may be the number of
levels (`rb_muzzl` has only `.mp0`).
