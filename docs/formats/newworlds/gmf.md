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

Checked on the 7 maps of `icewrld.cat`:

- A tile's type-20 header's third value is the number of its ground model
  among the `.geo` files of the map's archive, in archive order (counting from
  0, `lod/` models included). Shifting the numbering by one in either
  direction halves how many edges meet, so this is right.
- **Rotation:** bits 14-15 of the fourth value turn the model anticlockwise
  by that many quarter turns about the tile's centre. Bits 16-17 take three
  values per model family (0-3, 4-7 or 8-11 for the whole code); they aren't
  a mirror or a height offset (tested). Each model appears in groups of 4
  records per rotation (`f1`: most models 16 times), which differ in their
  texture layers.
- **Layout:** a block's four tiles go round the square in record order, at
  (0, 0), (1, 0), (1, 1), (0, 1) tiles from its corner. With rotation, 88.5 %
  of neighbouring tile edges in `f1` meet exactly (heights at 5 points within
  80 units); the next best layout scores 72 % and no rotation 46 %.
- Ground models span -4096..4096 in x and y (a tile is 8192 units, a block
  16384). Heights come in levels built into the models: 0, 3264 and 7360
  (`ice_08` is a plateau at 3264, `ice_09` at 7360; cliffs and ramps join
  them). Most remaining mismatches are flat tiles at 0 beside `ice_09`
  plateaus, even inside one block, and nothing tested (cell values, block
  bytes, tile fields) lifts them: perhaps the game draws walls between levels.
- In each type-21 record, the first four items count 0-3 starting at
  (rotation + 2) mod 4: the tile's sides, numbered in its own frame.

## Texture layers

A type-20 record's five 8 x 8 grids are texture layers: each cell holds a
texture number, the planet archive's full-size textures (`.mp0` and `.lbp`)
in archive order (`icewrld`: 0 `blue01` ... 15 `ice01` ... 30 `hole02`, 31
`tree.lbp` ...), or 0 for none. Higher layers cover parts of a tile in
staircase shapes; the 8 bytes after the grids are bit masks of rows
(`ff ff fc f0 f0 e0 c0 c0`), presumably for the top layer. The first row and
column of a layer hold a different value (`0f` around `09`, `02` around
`0e`): unknown. Painted from above with each texture's average colour, the
cells' values follow the landscape's shapes, the first-row values don't.

The viewer paints each pixel with the topmost layer's texture in its cell
(cell from the tile's texture coordinates); that comes out banded, so the
layers are probably blended (by the masks?) or the cells are oriented
differently. G switches to the models' own textures.

A ground model's own 8 x 8 grid (after its faces) lists, per cell, the faces
over it; cells overlap, so it's a lookup for heights or collisions, not
textures.

## Open questions

- How the texture layers combine; what the first row and column hold.
- What closes the gaps between plateaus and flat tiles beside them.
- Bits 16-17 of the tile flags; the rest of the type-21 records; the grid
  entries' second value (50, 75, 100, ... in steps of 25, sometimes with
  bits 12-15 set) and third (multiples of 4096); the list entries (probably
  placed scenery: the planet archives hold `tree`, `rock`, `bush` and
  `frn_spec` models); where units and buildings are placed.
- The data after the records (most of the file).
