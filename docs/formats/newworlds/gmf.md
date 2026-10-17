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
| 21 | 148–155 | one per tile (see below): small items `00 00 00 k 00 80 00`, many `02 ff ff ff ff ff` |
| 20 | 338–362 | one per tile: u32 its own number (0, 1, …), u32 0, u32 ?, u32 `0x18000`, u16 ?, then 8 × 8 byte grids (5, plus 0–3 8-byte items); values such as 2, 8, `0b`, `0d`, `0e`, perhaps terrain textures per cell |
| 19 | 22 | a 2 × 2 **block** of tiles: 6 bytes (0), then four u32 tile numbers, e.g. (0, 1, 3, 2), (4, 5, 6, 7), (8, 5, 6, 7) |

So the type-21 and type-20 counts are the number of distinct **tiles** (each
with two records), type 19 builds 2 × 2 **blocks** from them, and the header's
grid is in blocks: `kurt64` is 32 × 32 blocks = 64 × 64 tiles, as its name
says. Ground-tile models (`geo/map/...`) also have an 8 × 8 face grid.

After the records (checked on `kurt64`, from the file itself):

```
u32 N                         37092 in kurt64, 39607 in coverd2
N × u32                       lists, each ended by ff ff ff ff; entries are
                              24-bit values with bit 23 set, e.g. 00a0b277
grid w × h × 10 bytes         one entry per grid cell, row by row:
                                u16 block number (0-5 in kurt64)
                                u16 50 in kurt64 (all cells; 0 in coverd2's
                                first ones), perhaps a height
                                u16 0
                                u32 where the cell's list starts in the N u32s
1024 bytes                    mostly 0; starts with a few 24-bit values
                              (00ff00ff, 0042c6f9, 004175a5, …)
```

The lists are stored column by column (cell (0, 1) starts at 47, cell (1, 0)
at 1380). `kurt64`'s grid is a field of block 1 with scattered variants
(blocks 2-5) and a patch of block 0, as a test map would be.

The list entries' middle byte and bits 19-22 are evenly spread over all
their values, and their low byte has a strongly patterned distribution, so
they are packed fields of some kind; what they describe isn't known yet
(placed scenery, pathfinding or visibility data?).

## Tiles and ground models

A tile's type-20 header's third value is the number of its ground model
among the `.geo` files of the map's archive, in archive order (counting from
0, `lod/` models included). In `kurt64` (`icewrld.cat`): 6 = `ice_07` for the
field (tiles 4-7, block 1), 30 = `crack_08` for the crack tiles (8-11, which
blocks 2-5 each swap in once), and 32 = `hole_02` for the patch of block 0
(tiles 0-3). The fourth value differs per kind too (`0x18000`, `0x8000`,
`0x1018000`).

Ground models span -4096..4096 in x and y, so a tile is 8192 units and a
block 16384. Their edges are at height 0 where they meet (`ice_07` on all
sides; `crack_08` except where its crack reaches two of them). Their faces
pick textures the usual way (the last value of the last corner); `ice_07`
uses one, `crack_08` four. The planet archives hold textures named like the
models without the underscore (`ice07`, `hole02`, `brid01`), presumably each
model's first; the others aren't known.

A ground model's own 8 x 8 grid (after its faces) lists, per cell, the faces
over it; cells overlap, so it's a lookup for heights or collisions, not a
texture map. The type-20 grids are the same (2 on the edge, `0b` inside) for
all of `kurt64`'s tiles, whatever their model, so they aren't textures
either; perhaps whether ground is passable.

The viewer places a block's four tiles round the square in record order
(x, y offsets (0, 0), (1, 0), (1, 1), (0, 1)), which matches block 0's
numbering (0, 1, 3, 2) but isn't confirmed; on `kurt64`, where the edges are
flat, any order looks the same.

## Open questions

- What the type-21 records, the type-20 grids and the list entries hold;
  how a block's tiles are arranged (and whether tiles can be rotated); the
  grid entries' second value (50 everywhere in `kurt64`: a height?); where
  units and buildings are placed.
- The data after the records (most of the file).
