# Star Trek: New Worlds: the training missions recorded in a VM

The project's own recordings of the Federation training missions, played
in a virtual machine (October 2026): **"Tutorial - Tutorial"** (17:02, C1)
and **"Tutorials - Others"** (100:19, C4 to C8 one after another, without
going back to the menu). All six were completed. Unlike the YouTube
recording reviewed in [play-tutorials.md](play-tutorials.md), there is no
commentary and no editing, so the times are the game's own; the sound is
poor (after about 9 minutes of the second video no voice clip can be
matched), so most messages are read from the screen.

The details, mission by mission:

| Page | Video stretch | Missions |
|---|---|---|
| [play-vm-c1.md](play-vm-c1.md) | Tutorial 0:00-17:02 | C1 Tutorial (45 messages; the options and about screens) |
| [play-vm-c4-c6.md](play-vm-c4-c6.md) | Others 0:00-33:06 | C4 Building, C5 Mining Resources, C6 Power |
| [play-vm-c7.md](play-vm-c7.md) | Others 32:55-55:33 | C7 Defense |
| [play-vm-c8.md](play-vm-c8.md) | Others 55:15-100:19 | C8 Combat |

## What they settle

### Build times

From the order (the click on a green hologram; the stock drops at once) to
the finished building, in seconds. The dark pad appears 4-10 s after the
order. Low power didn't slow building: C6's first generators went up in
23 s at 20 % demand met, like the later ones at 100 %.

| Building | C1 | C4-C6 | C7 | C8 | Used |
|---|---|---|---|---|---|
| Resource processor | | 23 | | | 25 |
| Power generator | | 23-26 | 28-38 | 40-45 | 30 |
| Construction yard | 44 | 43 | | 46 | 45 |
| Mining station | | 50-60 | 67-80 | 55 | 60 |
| Vehicle yard | | | 66 | 64 | 65 |
| Science station | | | 76 | | 75 |
| Security center | | | 83 | | 83 |
| Hub upgrade | 40 | 40-50 | 45-54 | | |
| Construction yard upgrade | | 16-25 | 15-25 | | |
| Vehicle yard upgrade | | | 47 | | |
| Science station lab | | | 21-35 | | |
| Processor plant (two at a time) | | 15-27 | | | |

Vehicles from the vehicle yard: ground scout about 8 s, armed transport
about 25 s, photon artillery and phaser tank 40-50 s.

### Costs

Every build menu cost, from the hover tooltips and checked against the
stock's drops (T, G and KG are one stock unit each). Order: dilithium,
polycomp, thermoconc, trans al, nitrium, duranium.

| Item | Cost |
|---|---|
| Power generator | 35, 1, 35, -, 800, - (C7's tooltip reads 25, but the small font draws 3 like 2 or 5; C8's stock dropped exactly 35) |
| Resource processor | -, 4, 30, 2, -, - |
| Construction yard | -, 12, 89, 5, 50, - |
| Vehicle yard | -, 10, 90, 5, -, - |
| Mining station | -, 14, 80, 10, -, 140 |
| Science station | -, 4, 53, 17, 120, - |
| Sickbay | -, 8, 48, 20, -, - |
| Transporter pad | -, 6, 32, 2, -, - |
| Security center | -, 10, 100, 10, 1200, - |
| Storage facility | comes with the colony; no cost |
| Improved hub | -, 4, 40, -, -, 1000 |
| Advanced hub | -, 50, 20, 20, -, - |
| Improved yard (construction) | -, 2, 20, 4, -, - |
| Advanced yard (construction) | -, 20, 10, 6, -, - |
| Extended / specialist yard (vehicles) | -, 1, 50, 3, -, - each |
| Science station lab (each) | -, 10, 15, 7, -, - |
| Processor plant (each) | -, 2, 12, 10, -, - |
| Construction, cargo bee | 1, 1, -, 1, -, - |
| Work bee | -, -, -, 1, -, - |
| Ground scout | 2 or 3, 1, -, 1, -, - |
| Armed transport | 4, 3, -, 1, -, 30 |
| Photon artillery | 10, 16, -, -, 100, 60-80 |
| Advanced artillery | 12, 18, -, -, 150, 90 |
| Phaser tank | 15, 12-19, -, 2, -, 100 |

The menu shows three items a page; what's offered grows with the
colony's stages (the security center once hub and yard are at stage 2,
the advanced artillery and a nitrium plant at stage 3, the phaser tank
after the phaser coil lab).

### Starting stock and colony size

| Mission | Stock | Colony size |
|---|---|---|
| C1 | 660, 160, 2300, 660, 5600, 6600 | 24/60 |
| C4 | 500, 1000, 2300, 500, 4000, 5000 | |
| C5 | 100, 1000, 2300, 500, 900, 5000 | 24/50 |
| C6 | 300, 1000, 2300, 500, 3950, 5000 | 25/50 |
| C7 | | 24/50 |
| C8 | | 20/50 |

### Colony size

The crew grew by about one every 1.5-3.5 minutes (C8: 20 to 35 in 38
minutes). The maximum never moved (60 in C1, 50 in the others), through
hub, yard and vehicle yard upgrades and every other building: none of
the six built accommodation or a transporter pad. What raises it is
still open (F1's rose 50, 55, 65, 80, 90). The construction yard's panel
has its own COLONY SIZE n % line (100-225 %), meaning unknown.

### Power

- The hub alone runs its generator at LOW: in C1 consumption 85-88 %,
  demand met 100 %.
- C6 starts at demand met 20-21 %, C7 at 14 %, C8 at 40 %. Each generator
  lifts it a step (C6: 72 % after the first); C6 needs five generators to
  finish, C7 three.
- A generator's own panel: MED, efficiency 120 %.
- Once demand is met the reserve fills at 1-2.3 % a second.
- The power warning repeats exactly 60 s apart (C8).

### Mining and materials

- Mining stations go on bare ground off the concrete; the hologram is
  green over a deposit and red over plain grass, and a mine can be placed
  from the full-screen Tricorder.
- One mine fed the processor about 1.1 raw dilithium a second plus about
  0.2 of each other ore (C7); the processor holds all six ores at once.
- Mines ran out 6-13 minutes after being built. Exhausted at <060:396>,
  <060:420>, <084:436>, <092:300>, <092:380>, <116:380> (C7, C8).
- Materials: in C5 all six rose together while both mines worked (about
  0.3 a second for the four counted in T or G, ten times that for nitrium
  and duranium); in C7 dilithium about 0.3 a second, nitrium and duranium
  3-5, polycomp and thermoconc flat. Talgonite piled up unused in C6 and
  C8 (413 to 717 in 5½ minutes). So which ore makes which material isn't
  one for one; still open.

### Placement and construction

- The hologram (and the pointer) turn red too close to a building or to
  parked vehicles, among trees and at the concrete's edge; green on free
  concrete. Every building but the mines went on concrete.
- Construction: 2-4 black construction bees fly in and work round the
  site; a dark pad spreads over the footprint, the walls rise as dark
  untextured pieces, then the texture is revealed piece by piece or in
  strips from front to back; the top goes on last. Upgrades are built the
  same way.

### The missions

- Every training mission ends with a debriefing loading box and a
  MISSION REPORT page (its objectives marked COMPLETE, a CONTINUE
  button), then the next briefing; after C1 and C8, the main menu.
- C1 has 45 messages; its report lists only the attack objective. C6 can
  be finished (five generators). C8's report lists two objectives.
- C1's colony is around <395-402:272-281>, on or beside dilithium; C4-C8
  share one site round rows 50-90, columns 350-435.
- C8 adds a message the YouTube recording lacked: vehicle building held
  up for lack of crew.
- The Options screen holds three volume sliders and the screen sizes
  640 x 480, 800 x 600 and 1024 x 768; the game ran at 640 x 480.
- In the F1 first-person view the COMMUNICATIONS box moves to the top
  centre by itself, and back in the other views.

## Still open

- Unit speed: no unit was read on the Tricorder at both ends of a drive
  (C8's group of 15 crossed about half the map in 75-80 s).
- Which ore makes which material; what raises the colony's capacity.
- The ground scout's dilithium (2 or 3) and the phaser tank's polycomp
  (12 or 18).
- The construction yard's COLONY SIZE % line.
- Why C6's demand met reached 100 % before its second generator was done.
