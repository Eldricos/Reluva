# Star Trek: New Worlds: starting colonies

The maps don't hold the colonies a mission starts with ([gmf.md](gmf.md)):
the game places them from its mission scripts inside `STNW.exe`. The
recordings of play ([play.md](play.md)) show them only in passing. This
page collects what they show, and how Reluva's missions page lays out an
approximate colony from it (`src/engines/newworlds/colony.h`).

## What the recordings show

- **F1** starts with the colony hub (`fch`) on a large concrete pad, the
  camera over it, the Tricorder reading 407:209, and four vehicles.
- **K1** starts with the colony hub (`kch`) on a concrete pad and three
  other buildings, which the recording doesn't name.
- **J4K** starts beside an old Klingon colony of four buildings to
  recapture. **J8K** starts with about a dozen buildings, and **C10**
  with its colony already built (52/50).
- Every other mission's start isn't seen closely enough to say. The
  colony size at the start varies (F1 20/50, F2 24/90, K2 35/90).

None of the recordings shows which way a colony faces.

## Where the colonies are

Only F1's position is read directly (the start's Tricorder readout). For
the rest, the first mines the colony reported exhausted stand in: mines
lie beside the colony. Where two did, the middle of them is used.

| Mission | Side | Position | From |
|---|---|---|---|
| F1 | Federation | 407:209 | the start's Tricorder readout |
| J2 | Federation | 208:424 | between mines 196:436 and 220:412 |
| F12 | Federation | 212:196 | a mine (reported late, at 31:28) |
| J9 | Federation | 312:392 | between mines 316:404 and 308:380 |
| C9 | Federation | 260:300 | a mine |
| K1 | Klingon | 244:92 | a mine (reported late, at 35:02) |
| K2 | Klingon | 300:284 | between mines 324:260 and 276:308 |
| J1 | Klingon | 52:148 | a mine |
| J2 | Klingon | 92:404 | a mine |
| J3 | Klingon | 204:172 | a mine |
| K7 | Klingon | 160:440 | between mines 172:444 and 148:436 |
| J4 | Klingon | 340:180 | the first of several mines |
| K12 | Klingon | 84:252 | a mine |
| K14 | Klingon | 100:292 | a mine (on K12's map) |
| J9 | Klingon | 308:368 | between mines 316:380 and 300:356 |
| C10 | Klingon | 292:364 | the first mine (the second, 276:444, is ten blocks away) |

The other missions (F2, F3, F8, F14, J1, J3, J4, J6 to J8 as the
Federation, and K8, J6 to J8 as the Klingons) have no position yet.

Every mine position the recordings give is a block's middle (8 × n + 4 in
both numbers: 404:220, 244:92, 300:284), so mining stations stand one to
a block, on its deposit.

## Buildings and models

The scripts' building names, and the models the recordings matched to
them ([play-f1.md](play-f1.md), [play-k1.md](play-k1.md)). Those marked
*name* are taken from the model's name alone and are guesses (`fpgs`,
`kpgs`: power generator station).

| Building | Federation | Klingon | Romulan |
|---|---|---|---|
| Colony hub | `fch` | `kch` | `rch` *name* |
| Colony hub upgrade | `fch_b` | `kch_b` | `rch_b` *name* |
| Vehicle yard | `fvpy_a` | `kvy` | `rvy` *name* |
| Construction yard | `fcy` | `kcy` | `rcy_a` *name* |
| Resource processor | `frps_a` | `krps` | |
| Power generator | `fpgs` *name* | `kpgs` | `rps1` *name* |
| Advanced generator | `fpgs2` *name* | `kpgs2` | `rps2` *name* |
| Mining station | `fms` *name* | `kms` | `rms` *name* |
| Accommodation | `accom` | | |
| Sickbay | `fsb` *name* | `ksb_a` | |
| Science station | `fss_a` *name* | `kss_a` | |
| Science station upgrade | `fss_b` *name* | `kss_b` *name* | |
| Security center | `fsc` | | |
| Hydroponics / livestock farm | `fhf` | `klf` | |
| Storage facility | | `ksf` | |
| Arsenal | | `kab` | |
| Transporter pad | `landpad` *name* | | |
| Photon turret | `fpdt` *name* | | |
| Disruptor turret | | `ksg` | |
| Static disruptor | | `kdws` *name* | |

Most Klingon buildings have a texture number the name rule can't fill
([geo.md](geo.md#textures)), so a face of each (often its top) shows
untextured.

## Reluva's layout

The missions page (**C** shows or hides the colony, **V** looks at it) lays
out a colony like this. It is Reluva's own approximation, not the game's
layout:

- Buildings stand on the map's tile grid (half a block, 4 map positions),
  each over as many tiles as its model needs with a small gap, centred on
  its model's middle (some models' origin is at a corner: `rch`, `rms`).
- A tile is free if nothing stands on it, its ground is flat, near the
  hub's height and on the map, and its block has no deposit.
- The hub goes within six tiles of the site, where the most free tiles
  are around it, less a little for each tile it moves.
- Each building the mission's script builds (its `built` events, as **X**
  steps through them) goes on the free place nearest the hub; a mining
  station goes on the nearest deposit instead, and an upgrade on the
  building it upgrades.
- Trees and rocks under a building aren't drawn.

What would settle the real layout: a starting colony seen from above in
the running game, for any mission.
