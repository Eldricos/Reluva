# Star Trek: New Worlds: `.geo` models

Status: **core geometry understood** (faces, texture coordinates, vertices,
normals, edges); several blocks are still skipped. Worked out from complete
byte dumps of five small models, October 2026:

| Model | Bytes | What it is |
|---|---|---|
| `geo/rom/rb_mid.geo` | 980 | beam section: a two-sided rectangle |
| `geo/fed/fs_s.geo` | 1268 | flat outline: a hexagon on one side, 4 triangles on the other |
| `geo/kln/kwb_s.geo` | 1440 | two separate flat pieces |
| `geo/map/aln/concrete.geo` | 1452 | terrain tile: flat 8192 × 8192 square |
| `geo/map/aln/lod/alien_14.geo` | 1476 | terrain tile: square sloping up 3264 units |

Reader: `src/engines/newworlds/geo.cpp`. Check every model with
`reluva-tool geo-survey *.cat`; export one to OBJ with
`reluva-tool geo-info <file> <out.obj>`. Stored in `.cat` archives with chunk
tag `0x17`; the size in the archive's copied metadata never matches, so these
were presumably converted when packed.

All values little-endian; floats are IEEE 32-bit.

```
faces          face count × face; face 0's header is the file header:
                 u32 ?        file header: 952 / 416 / 1072 / 328 (fs_s, rb_mid, kwb_s, terrain)
                              other faces: 0 so far
                 u32 ?        file header: the face count; other faces: 0 or 1
                 u32 corners
                 u32 0
                 corners × 32 bytes: u32 vertex, f32 u, f32 v (both × 65536), 20 bytes
bounds         u32 ?, u32 ?, u32 1, u32 attachment count (2 in rb_mid, else 0)
               f32 centre x, f32 centre y, f32 radius
               u32 1, 0, u32 ? (1 in rb_mid, else 0)
...            attachment data, layout unknown (116 bytes in rb_mid)
vertices       u32 ?, 1, 1, 0, u32 count, count × (f32 x, y, z)
normal lists   repeated while present: u32 1, u32 count, count × unit vector
edges          u32 0 = none (terrain tiles), or u32 1, u32 group count, then
               per group: u32 count, count × (u32 0, v0, v1, faceA, faceB)
...            terrain tiles: u32 1, u32 64, then per cell of an 8 × 8 grid a
               count and that many face indices; then u32 8, 8, ?, bounds, and
               the faces again with another set of texture coordinates
trailer        names ("main", "terminate", "muzzle_flash", "front_lock"),
               layout not studied
```

## Evidence

- **Face 0 is in the file header.** In `concrete` the header's `2, 3` are 2
  faces and 3 corners, and the first 3 records form a triangle (1, 3, 0) with
  the second triangle (2, 0, 3) after it. Read that way, `rb_mid` is a
  rectangle (3,1,0,2) and its back (3,2,0,1), and `fs_s` is a hexagon and the
  four triangles of its other side.
- **Corners** carry their own texture coordinates; in `rb_mid` they form a
  consistent planar mapping (u along y, v along x) only if the first field is
  the vertex index.
- **Edges** name two vertices and the two faces either side, 0-based. Every
  edge in all five models passes the reader's check that both vertices lie on
  both faces: e.g. all four edges of `rb_mid` lie between face 0 and face 1
  (front and back), and `fs_s` has the 9 edges between its hexagon and its 4
  triangles.
- **Edge groups**: `kwb_s` has two groups of 5 edges, one per piece
  (vertices 0–3 with faces 0–2, and 4–7 with faces 3–5). Only with two groups
  does it show that each record starts with the 0 rather than ending with it.
- **Radius** is the farthest vertex from the centre: 488.2 in `fs_s` is the
  distance to (−88.6, −480.1).

## Open questions

- The file header's first value, face header values, and the 20 bytes after
  each corner's texture coordinates (sometimes a 1 in the last word).
- Attachment data; the terrain grid's exact layout; the second texture set in
  terrain tiles; the trailer.
- What the two normal lists are (vertex and face normals?).
- Which texture a model uses (not stored in the model as far as seen).
- Larger 3D unit models haven't been checked yet.
