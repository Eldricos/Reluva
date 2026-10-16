# Star Trek: New Worlds: textures (`.lbp`, `.mp0`–`.mp4`, `.eb0`, `.eb1`, `.fbp`)

Status: **hypothesis**, from a hex dump of `lbp/txr/rom/rom_hs.lbp` and the
sizes of every texture in `rommisc.cat` and `glblgame.cat`. Check one with
`reluva-tool lbp-info <file> <prefix>`, which decodes it to BMP if the layout fits.
Stored in `.cat` archives with chunk tag `0x16`.

```
u32 format?          6 in rom_hs.lbp
u32 palette count?   256
palette              count × 4 bytes, e.g. d2 ce ce 00 (channel order not yet known)
u32, u32, u32        image header, probably width, height and one more value
pixels               width × height bytes, one palette index each
```

Evidence: every texture size seen so far is 8 + 1024 + 12 + a power of two:
9236 (8192), 33812 (32768), 66580 (65536), 132116 (131072), 3092 (2048),
2068 (1024). A synthetic 64 × 32 texture built to this layout is 3092 bytes,
the size of the real `lns_brn.lbp` and `lns_blue.lbp`.

## Texture sets

Container tag `0x29` holds an `.mmb` (tag `0x2a`) followed by one or more
texture levels (`.mp0`, `.mp1`… or `.eb0`, `.eb1`). `rb_muzzl.mmb` is
`01 01 00 00` followed by the float 0.25; the second byte may be the number of
levels (`rb_muzzl` has only `.mp0`).
