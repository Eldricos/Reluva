# Star Trek: New Worlds: `.gmf` maps

Status: **first look**. Stored in the planet archives (`alnwrld`, `alpwrld`,
`deswrld`, `icewrld`, `redwrld`, `vlcwrld`) with chunk tag `0x11`, 3 to 7 per
archive, mostly 380–470 KB (`kurt64.gmf` in `icewrld` is 166 KB). Each
archive also holds that planet's ground tiles (`geo/map/alp/alpin_01.geo`
…, 93 in `alpwrld`) and terrain textures (`lbp/txr/map/alp/green01`,
`cliff01`, `brid01`, …). Worked out from the start of `coverd2.gmf`, October
2026. `reluva-tool gmf-info <file>` walks the layout below.

All values little-endian.

```
0x00   u16 × 5    376, 376, 749, 64, 64 in coverd2 (a 64 × 64 grid?)
0x0a   records:   u16 type, u16 length, u16 ? (0 so far), length bytes
```

The first records of `coverd2` are all type `0x15`, length 152 (later 154,
153). Their bodies start with four 7-byte items `00 00 00 k 00 80 00`
(k = 0..3), then repeat the 6-byte item `02 ff ff ff ff ff`, probably empty
cells along the map's edge; the first eight records are identical.

## Open questions

- Everything: what the header values and records mean, how cells refer to
  ground tiles and textures, and where units and buildings are placed.
