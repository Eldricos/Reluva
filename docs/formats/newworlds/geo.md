# Star Trek: New Worlds: `.geo` models

Status: **partly understood**, from complete byte dumps of two small models,
`geo/rom/rb_mid.geo` (980 bytes, a beam section: a flat 4-cornered rectangle)
and `geo/fed/fs_s.geo` (1268 bytes, a flat 6-point outline), October 2026.
Reader: `src/engines/newworlds/geo.cpp`. Check the description against every
model with `reluva-tool geo-survey *.cat`; export one with
`reluva-tool geo-info <file> <out.obj>`.
Stored in `.cat` archives with chunk tag `0x17`. The size in the archive's
copied metadata never matches, so these were presumably converted when packed.

All values little-endian; floats are IEEE 32-bit.

```
header            u32 ?            952 in fs_s, 416 in rb_mid
                  u32 faces + 1    5 / 2
                  u32 n            texture vertex count: 6 / 4
                  u32 0
texture vertices  n × 32 bytes: u32 vertex index, f32 u, f32 v, 20 bytes (0 so far)
faces             (faces) × { u32 ?, u32 ?, u32 corner count, u32 0,
                              corners × 32 bytes, laid out like texture vertices }
bounds            u32 0, 1, 1, u32 attachment count (2 in rb_mid, 0 in fs_s)
                  f32 centre x, f32 centre y, f32 radius
                  u32 1, 0, u32 ? (1 / 0)
...               attachment data, layout unknown (116 bytes in rb_mid)
vertices          u32 ?, 1, 1, 0, u32 count, count × (f32 x, y, z)
normal lists      repeated while they appear: u32 1, u32 count, count × unit vector
edges             u32 1, 1, count, 0, then count × (u32 v0, v1, faceA, faceB, 0)
trailer           a few u32s, then names ("main", "terminate", "muzzle_flash",
                  "front_lock"), layout not studied
```

Texture coordinates are stored × 65536 (`128` ≈ 0.002, `65410` ≈ 0.998).

## Evidence

- **Texture vertices refer to 3D vertices.** In `rb_mid` the coordinates
  only form a consistent planar mapping (u along y, v along x) if the first
  field is the vertex index.
- **Face corners** list vertex indices in order, each with its own texture
  coordinates. `fs_s` has four triangles (1,0,2), (5,3,1), (1,4,5), (1,2,4)
  covering its 6-point outline; `rb_mid` one quad (3,2,0,1).
- **Edges** are (v0, v1, faceA, faceB) with 1-based faces and 0 for none:
  `fs_s` has exactly the 9 edges of its triangulation, e.g. 2-1 between faces
  1 and 4; `rb_mid` the 4 sides of its quad, all between faces 0 and 1.
- **Radius** is the farthest vertex from the centre: 488.2 in `fs_s` is the
  distance to (−88.6, −480.1).
- The header's second value is one more than the number of face groups in
  both files.

## Open questions

- The first header value; the face header's first two values (0 and 0/1 so
  far); the per-corner value at +12 (1 in `rb_mid`, 0 in `fs_s`).
- The attachment data and the trailer with names.
- The two normal lists (vertex and face normals?).
- Which texture a model uses (not stored in the model as far as seen).
- How multi-part 3D models are organised; both files studied are flat.
