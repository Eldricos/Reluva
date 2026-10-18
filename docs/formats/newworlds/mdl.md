# Star Trek: New Worlds: `.mdl` metamodels

Status: **layout of the names and states known; the geometry after them is
the `.geo` format**. Stored in `misc.cat` with chunk tag `0x2c` (522 files):
ships, vehicles, crew and buildings (`mdl/kln/k_bop.mdl`, `mdl/fed/fss_a.mdl`),
the planets' ground tiles (`mdl/ice/ice07.mdl`), sky and solar-system models
(`mdl/sky`, `mdl/solsys`) and a few others. Worked out from the files
themselves, October 2026. `reluva-tool mdl-info <file.mdl | misc.cat>` shows
one or surveys them all; all 522 decode.

A metamodel names a model's parts and points and lists its states; each
state's geometry follows, as in the `.geo` files (which look like those
geometries exported one by one). All values little-endian.

## Layout

```
char[32]  title, e.g. "klnbirdofpreymetamodel", "tileice07metamodel"
4 name lists, each:
    u32 count (can be larger than the entries written; see below)
    entries: char[32] name, u32 value
    an entry named "terminate" (value -1) ends the list
u32 0       (absent in crewman and conbee)
u32 state count
states: char[32] name, u32 value, up to "terminate" or the count
            (karazi has no "terminate" after its one state)
the states' geometry and other data (below)
```

When the first list's count is larger than the entries written (`k_bop`:
12 and 10; `fss_a`: 15 and 1), 36-260 bytes of what look like saved memory
addresses (`0x0099xxxx`) follow it before the second list; their first
value varies (`0x818`, `0x188`, …). They seem to be leftovers of deleted
entries; a reader skips to the next count followed by a name. Unused bytes
of name fields hold other leftovers ("ariantary", "ice07geo").

## The lists

| List | Holds | `k_bop` | `fss_a` (science station) | `crewman` |
|---|---|---|---|---|
| 0 parts | nodes of the model: name → node number | `main`=0, `turret_1`=1, `gun_1`=2, … `gun_3`=6, `muzzle_flash_1`=2 (on its gun) | `main` | 13 body parts: `head`, `main`, `pelvis`, `left_thigh`, … |
| 1 points | the model's attachments, by number in the `.geo` | `turret_axis_1..3`, `gun_elevate_1..3`, `gun_muzzle_1..3` (9) | `BAY_0..5_LOCK`, `VEHICLE_DOCK_CAPTURE`, `_LOCK`, `_ARRIVAL`, `_RELEASE`, `_DEPARTURE`, `PORTAL` (12) | none |
| 2 places | named places (effects, docking bays) | `muzzle_flash_1..3` | `BAY_0..5`, `VEHICLE_DOCK` | none |
| 3 animated | parts that animations move | none | none | the same 13 parts |

The node numbers in list 0 aren't `.geo` part numbers (`k_bop.geo` has 3
parts, its list 0 numbers up to 6); how nodes map to parts isn't known.
`conbee` (construction vehicle) has parts `main`, `arm`, `boom`, `grabber`,
point `polygon_grab`, and animated `arm`, `boom`, `grabber`.

## States

Across `misc.cat`:

| States | Files |
|---|---|
| `normal`, `low detail` | 292 (ships, vehicles, ground tiles) |
| `constructed`, `deconstructed` | 87 (buildings) |
| one unnamed state | 130 (interiors `*_p`, effects, …) |
| `reference`, `stand`, `walk` | 3 (crew) |
| `reference`, `manipulate` | 3 (construction vehicles) |
| `mid_section`, `mid_tail_section`, `mid_head_section`, `mid_head_tail_section` | 3 |
| `normal`, `low_detail` / `normal` | 4 |

The animated models' states are presumably animations; `kliactor.cat`
holds `.arf` files (tag `0x25`, e.g. `conbee00.arf`-`conbee15.arf`), not yet
studied.

## Geometry

After the states: u32 values (`0, 0, 0, 2` in `ice07` and `k_bop`), then
each state's model, the same bytes as its `.geo` without the `.geo`'s final
name lists (216 bytes in `ice_07.geo`, 1040 in `k_bop.geo`: the names are in
the metamodel instead). In `ice07.mdl`, `ice_07.geo` starts at `0x168` and
`lod/ice_07.geo` at `0x1b1c`; in `k_bop.mdl`, `k_bop.geo` at `0x4a4`,
`k_bop_s.geo` (its shadow) at `0x1868e`, and a second, identical copy of
`k_bop.geo` (its `low detail` state) at `0x1b36a`. Each model is followed by
short records such as `0, 1, 14, 0, 0, 0x0500007f` (`ice07`) or `0, 3, (14,
0, 0, 0x0c000005), (14, 0, 0, 0x0c000009), …` (`k_bop`), not yet understood.

No texture names appear in any metamodel, ground tiles included.

## Attachments in the geometry

The points' positions are in the geometry, in what
[geo.md](geo.md) calls the attachment block, after the bounds:

```
u32 node count       the number of entries in list 0 (k_bop 10, crewman 13)
u32 place count      the number of entries in list 2 (k_bop 3)
u32 ?                1 when there are attachments
attachment count × 58 bytes:
    40 bytes         probably the point's orientation (layout not known)
    u16 0x1c, u32 12, 3 × f32 position
```

So the block is 12 + 58 × the attachment count bytes (true for all 304 `.geo`
files checked), and the attachment count is list 1's. In `k_bop` the wing
guns' points are at (±4022, −3180, −3327) and the nose gun's at (0, −4206,
−401); each gun's axis, elevation and muzzle points share one position.

## Open questions

- How list 0's nodes map to the geometry's parts, and the turrets' pivots.
- The 40 bytes before each attachment's position.
- The records after each state's geometry; the values before the first.
- The `.arf` animations.
