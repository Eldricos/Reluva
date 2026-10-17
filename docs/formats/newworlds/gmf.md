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
0x00   u16 count of type-21 records   376 in coverd2, 12 in kurt64
0x02   u16 count of type-20 records   376, 12
0x04   u16 count of type-19 records   749, 6
0x06   u16 grid width, u16 height     64 x 64, 32 x 32
0x0a   the records, in that order: u16 type, u16 length, u16 ? (0 so far),
       then length bytes
...    data of unknown layout up to the end of the file
```

| Type | Length | Notes |
|---|---|---|
| 21 | 148–155 | bodies of small items: `00 00 00 k 00 80 00`, and many `02 ff ff ff ff ff` (empty?) |
| 20 | 338–362 | |
| 19 | 22 | |

The counts of types 21 and 20 are equal, so they probably describe the same
things in two ways.

## Open questions

- What the records describe, how cells refer to ground tiles and textures,
  and where units and buildings are placed.
- The data after the records (most of the file).
