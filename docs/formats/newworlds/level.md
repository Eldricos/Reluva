# Star Trek: New Worlds: `level.dat`

Status: **fully understood**. Reader: `src/engines/newworlds/levels.cpp`.
A loose text file in the game folder (1,134 bytes, CRLF lines): the
missions the main menu offers. `olevel.dat`, `kapla.rse` and `game.ini` are
covered below. `reluva-tool level-info <level.dat>
[planet.cat...]` lists them, with the archives holding their maps.

```
// format for levels displayed in the main menu
// parser short name, max players, text description
// --------------------------------------------
F1,2,Power Corrupts
K1,2,The Insult
...
```

One mission per line: short name, maximum players, title. `//` starts a
comment; some comments are disabled missions (an early `J1,6,Defiant`, and
the bonus missions C9-C11). 37 missions are listed, plus those 4:

| Short names | Missions |
|---|---|
| `F1`-`F14`, `K1`-`K14`, `R1`-`R14` | Federation, Klingon and Romulan campaigns (not every number; 12 and 14 are "Death Rattle" and "The Finest Hour" for all three) |
| `J1`-`J9` | joint missions, 3 players |
| `Test` | "Constraints" |
| `E1`-`E3` | "ECTS missions" (a trade show): Heisenberg, Excalibur, Constitution |
| `C1`, `C4`-`C8` | the tutorials (C1, 1 player); `C9`-`C11` the commented-out bonus missions |

## What the short name ties together

The short name is the mission's key across the files:

- **its map:** `map/<name>.gmf` in one planet archive (`F1` → `map/f1.gmf`).
- **its text:** `str/eng/miss/` in the side's misc archive: `F1` → `fed01.stb`,
  `J1` → `jnt01fed.stb` (the Federation side of a joint mission). All the
  C missions share `cvr03.stb`: training ("Duty first.", C1), the five
  tutorials (C4-C8) and C9-C11, in level.dat's order in the file, though by
  id "Slaver's Run" (C9, ids 57-61) sits between the fourth and fifth
  tutorials. The tables'
  titles match level.dat's ("A Piece of paper." for "A Piece of Paper").

The maps found so far:

| Planet archive | Maps | Briefings describe |
|---|---|---|
| `icewrld.cat` | `f1`, `j1`, `k7`, `r8`, `k12`, `r12` (+ `kurt64`) | ice world (F1), snow-covered world (J1) |
| `deswrld.cat` | `f2`, `r9`, `j7` (+ `demo1`) | desert world (F2) |
| `alpwrld.cat` | `f3`, `j3`, `f12` (+ `coverd1`-`3`) | forest world (F3), temperate world (F12) |

The missing maps are presumably in the planet archives not yet looked at,
going by their briefings: the volcanic world of J6 in `vlcwrld`, and the
dust world (F8) and barren world (J2) in `redwrld` or `alnwrld`. `kurt64`,
`demo1` and `coverd1`-`3` aren't in level.dat. `cvr01.stb` and `cvr02.stb`
("Careless Talk" on the arid world Varru, "A Friendly Little War" on the
ice world Dukalion) may belong with the `coverd` maps (cover-disc demos?),
though the `coverd` maps are in the alpine archive.

## The other lists: `olevel.dat` and `kapla.rse`

Two more files in the same format sit beside it, perhaps lists from other
builds. `olevel.dat` (1,241 bytes, presumably "old level") is `level.dat`
with the bonus missions enabled under other short names (`U9`-`U11` instead
of `C9`-`C11`) and six more; `kapla.rse` (1,346 bytes) has the same, three
odd test entries (`U2,2,LOSTYOU`, `U3,3,LOSTME`, `U3,2,WHATARUE`) and the six
renamed:

| Short name | `olevel.dat` | `kapla.rse` | Planet archive |
|---|---|---|---|
| `D1` | Forest of Brad | Head to Head: Lupercal Jade | `alpwrld` |
| `D2` | Ice of Brad | Head to Head: Nemphos | `icewrld` |
| `D3` | Volcano of Brad | Head to Head: Lestrigon | `vlcwrld` |
| `D4` | Desert of Brad | Head to Head: Karalute | `deswrld` |
| `D5` | Alien of Brad | Head to Head: Xix Xix | `alnwrld` |
| `D6` | Red of Brad | Head to Head: Pellion Red | `redwrld` |

That names the six planet types, one per world archive; the archive column
follows from the names (and, for `alpwrld`, from F3's "forest world" being
in it). No `d1`-`d6` maps are in the three planet archives seen.

## `game.ini`

```
[STNW]
GAMEXRES= 640
GAMEYRES=480
FedLevel=1
KliLevel=2
RomLevel=3
TauLevel=4
MetLevel=5
HubLevel=6
```

The screen size (640×480, the size the data viewers draw at), and a number
for each side: the three playable races, then the native races the
missions' text names: the Taubat, the Metar ("the Metar colony core") and the
Hubrin, whose buildings and units are in `forbuild.cat` (see
[geo.md](geo.md)). What the numbers are used for isn't
known; they may be the sides' player numbers in the missions.

## Not here

level.dat names missions but says nothing about how they're set up: the
planet, starting colonies, units, objectives' triggers and the voice-over
slots are elsewhere (presumably `STNW.exe`).

## In Reluva

The engine reads level.dat from the game folder, and the map viewer's title
names each map's mission (`map/f1.gmf [F1 "Power Corrupts", 2 players]`).
