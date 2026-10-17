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
1024 bytes                    a colour table: u32 entries, bytes B, G, R, 0,
                              ending with a zero entry (see Terrain codes)
```

The lists are stored column by column (cell (0, 1) starts at 47, cell (1, 0)
at 1380). `kurt64`'s grid is a field of block 1 with scattered variants
(blocks 2-5) and a patch of block 0, as a test map would be.

The list entries' middle byte and bits 19-22 are evenly spread over all
their values, and their low byte has a strongly patterned distribution, so
they are packed fields of some kind; what they describe isn't known yet.
They look like placed objects: in `f1`, bits 16-17 take 3 values (0, 1, 2:
perhaps the planet's tree, rock and bush models), bits 19-23 are spread over
16 values (perhaps a turn) and the low 16 bits look like a position.

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
  terrain codes. A few `ramp` tiles with bits 16-17 = 1 would fit better
  turned a further half turn; not enough to be sure.
- **Layout:** a block's four tiles go round the square in record order, at
  (0, 0), (1, 0), (1, 1), (0, 1) tiles from its corner. With rotation, 88.5 %
  of neighbouring tile edges in `f1` meet exactly (heights at 5 points within
  80 units); the next best layout scores 72 % and no rotation 46 %.
- Ground models span -4096..4096 in x and y (a tile is 8192 units, a block
  16384). Heights come in levels built into the models: 0, 3264 and 7360
  (`ice_08` is a plateau at 3264, `ice_09` at 7360; cliffs and ramps join
  them). Most remaining mismatches are flat `ice_19` tiles at 0 beside
  `ice_09` plateaus (1687 of 2900 in `f1`), even inside one block, and
  nothing tested (cell values, block bytes, tile fields, other rotations or
  mirroring) lifts them. The terrain codes (below) agree they are low ground:
  they are real pits in the plateau, so the game must draw walls there.
- In each type-21 record, the first four items count 0-3 starting at
  (rotation + 2) mod 4: the tile's sides, numbered in its own frame.

## Terrain codes

A type-20 record's five 8 x 8 grids are **not** textures, as first thought:
`hole_03`, `ramp` and `concrete` tiles, which look nothing alike, carry the
same grids, and `ice_09`'s plateau has the same codes in all five layers.
Each cell holds a code from 1 to 18, or 0 for none. Codes 1, 2, 4, 7, 10,
13, 15 and 18 appear only in a grid's first row and column, the others only
inside it, and they come in pairs (border 15 with inner 9, 2 with 11, 13
with 8, 4 with 12, 18 with 6, 1 with 17, 10 with 5).

The codes index the colour table at the end of the file (the same 18 colours
in every `icewrld` map; entry 0 is magenta). Coloured that way, plateaus are
light blue and lowland dark blue, and the first row and column draw a grid:
a minimap or terrain-type map. Cells are in map orientation (row 0 at the
south, column 0 at the west, after the tile's rotation). Higher layers cover
parts of a tile in staircase shapes; the 8 bytes after the grids are bit
masks of rows (`ff ff fc f0 f0 e0 c0 c0`) matching those shapes. What the
five layers are for (unit types? heights?) isn't known.

### Textures

Neither the map nor the ground models name their textures. Each face numbers
a texture among its model's own (0-6, as for other models; see
[geo.md](geo.md#textures)), with UVs across the whole tile (`hole_01`'s
crater spans 0-1). The planet's textures fit the models' names only in part:
`hole01`-`hole03`, `crack01`, `ramp01`, `concrete`, and `ice01`-`ice08` for
the 26 `ice_` models, several of which number more than one texture. Matching
textures to faces by colour across seams gave no consistent answer. The
assignment is probably held elsewhere (another archive or the game itself).
`cliff07`, which no model's name matches, may be the texture for the walls
between levels.

A ground model's own 8 x 8 grid (after its faces) lists, per cell, the faces
over it; cells overlap, so it's a lookup for heights or collisions, not
textures.

## Open questions

- Which textures the ground models' faces use.
- What the five layers of terrain codes are for.
- How the game builds and textures the walls between levels.
- Bits 16-17 of the tile flags; the rest of the type-21 records; the grid
  entries' second value (50, 75, 100, ... in steps of 25, sometimes with
  bits 12-15 set) and third (multiples of 4096); the list entries (probably
  placed scenery: the planet archives hold `tree`, `rock`, `bush` and
  `frn_spec` models); where units and buildings are placed.
- The data after the records (most of the file).
