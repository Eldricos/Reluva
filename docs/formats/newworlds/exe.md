# Star Trek: New Worlds: what the executable says

Facts learnt by studying `STNW.exe` (the user's own copy: PE32, built August
2000, not packed or protected), October 2026. The executable was
disassembled and decompiled with Ghidra, and its start-up initialisers were
run in an emulator to read the tables they build. Only facts are taken -
numbers, rules, data layouts - and written here in our own words; no code
and none of the game's text is copied, and nothing from the executable is
in the repository except the numeric tables generated from it
(`src/engines/newworlds/start_data.cpp`, and the costs in `economy.cpp`).
The analysis itself stays outside the repository. This is the same way
ScummVM studies original executables and extracts engine data from them.

Confidence: **sure** where a value and its use were both traced; *likely*
where the value is certain but its meaning comes from how one or two places
use it; *guess* otherwise. Unmarked figures are sure.

## The simulation clock

- The game advances in fixed steps of **50 ms** (20 a second); an
  options checkbox switches to 66 ms (about 15 a second; which option isn't
  known).
- The mission scripts count time in **sixteenths of a second**: a script
  timer of N seconds runs 16N of its units. The vehicle yard's build values
  (below) fit sixteenths too (128 = 8 s, which is what the tutorial's scout
  took). How the two clocks relate isn't settled.
- Warnings are paced in real time: the low-power warning at most once a
  minute (as measured in C8), "construction delayed" messages 0.6 s apart.

## Object types

Every building, vehicle, upgrade and effect has a type id from one global
list (318 entries). A name record per type gives its panel name and up to
four prerequisites (other type ids, upgrades included); a cost record gives
up to eight (material, amount) pairs and one more number, the **build
value**, which only the vehicle yard reads. Materials are numbered 1-6 in
the panel's order; amounts are kept in the game's units, a thousand per
stock unit for dilithium to trans al and a hundred for nitrium and
duranium (so 89000 is the panel's 89 T, 5000 nitrium its 50 KG).

Upgrades are separate types: Federation 159/160 improved/advanced hub,
161/162 improved/advanced construction yard, 146/147 extended/specialist
vehicle yard, 164 and 293-297 the science labs (shield buffer, tracking
systems, phaser coil, energy, sensor, away team), 165 and 298-301 the
sickbay wards, 166/167 the processor plants. Klingon: hub 168/169, yard
191/192, vehicle yard 183, labs 184-186 and 302-304, wards 187/188, 305,
306, plants 190. Romulan: hub 174/175, yard 176, vehicle yard 177, labs 179
and 307-311, wards 180 and 312-315, plants 181. Hubs: 23 Federation, 28
Klingon, 43 Romulan *likely* (not in the build menus).

## Costs and prerequisites

Panel units (T, G, KG all one stock unit each). The build value is the
vehicle yard's work, in what are probably sixteenths of a second at full
efficiency. Some tutorial readings were misread digits in the small font:
the generator's dilithium is 35, the improved yard's thermoconc 30, the
advanced hub's 30.

### Federation

| id | type | dilithium | polycomp | thermoconc | trans al | nitrium | duranium | build value | needs |
|---|---|---|---|---|---|---|---|---|---|
| 122 | storage facility |  |  |  |  |  |  | 0 | storage processor |
| 22 | power generator | 35 | 1 | 35 |  | 800 |  | 0 | construction yard |
| 77 | construction yard |  | 12 | 89 | 5 | 50 |  | 0 |  |
| 24 | vehicle yard |  | 10 | 90 | 5 |  |  | 0 | construction yard |
| 26 | storage processor |  | 4 | 30 | 2 |  |  | 0 | construction yard |
| 45 | mining station |  | 14 | 80 | 10 |  | 140 | 0 | storage processor |
| 171 | hydroponics farm |  | 18 | 75 | 40 |  |  | 0 | construction yard, accomodation block |
| 27 | landing pad |  | 6 | 32 | 2 |  |  | 0 | improved yard |
| 126 | science block |  | 4 | 53 | 17 | 120 |  | 0 | construction yard |
| 123 | sickbay |  | 8 | 48 | 20 |  |  | 0 | construction yard |
| 44 | accomodation block |  | 20 | 50 | 36 |  |  | 0 | improved yard, landing pad |
| 16 | security centre |  | 10 | 100 | 10 | 1200 |  | 0 | improved yard, improved hub |
| 80 | defensive weapon |  |  | 92 | 12 |  | 500 | 0 | improved yard, improved hub, shield generator |
| 21 | shield generator | 20 | 2 | 40 |  |  | 800 | 0 | improved yard, shield buffer lab, security centre |
| 83 | defensive artillery |  |  | 93 | 13 | 250 | 500 | 0 | advanced yard, advanced hub, security centre |
| 158 | advanced power generator | 50 | 2 | 40 |  | 850 |  | 0 | advanced yard, improved hub |
| 163 | advanced mining station |  | 18 | 92 | 15 |  | 1600 | 0 | advanced yard, advanced hub, storage processor |
| 118 | CFD |  | 6 | 110 | 52 | 300 |  | 0 |  |
| 13 | cargo bee | 1 | 1 |  | 1 |  |  | 128 | vehicle yard |
| 14 | construction bee | 1 | 1 |  | 1 |  |  | 0 | vehicle yard |
| 12 | worker bee |  |  |  | 1 |  |  | 0 | vehicle yard |
| 55 | unarmed APC | 5 | 10 |  | 2 |  |  | 0 | vehicle yard, away team lab |
| 62 | science vehicle | 8 | 14 |  | 2 |  | 80 | 640 | vehicle yard, sensor lab |
| 97 | ground scout | 3 | 1 |  | 1 |  |  | 0 | vehicle yard |
| 8 | phaser tank | 15 | 18 |  | 2 |  | 100 | 640 | extended vehicle yard, phaser coil lab |
| 56 | armed APC | 4 | 3 |  | 1 |  | 30 | 240 | vehicle yard |
| 47 | photon artillery | 10 | 16 |  |  | 100 | 80 | 640 | security centre, extended vehicle yard |
| 9 | advanced photon artillery | 12 | 18 |  |  | 150 | 90 | 640 | vehicle yard, specialist vehicle yard, security centre, science block |
| 154 | mobile shield generator | 16 | 20 |  | 2 |  | 160 | 640 | specialist vehicle yard, defensive weapon, defensive artillery, energy lab |

### Klingon

| id | type | dilithium | polycomp | thermoconc | trans al | nitrium | duranium | build value | needs |
|---|---|---|---|---|---|---|---|---|---|
| 61 | cargo bee | 1 | 2 |  | 1 |  |  | 0 | vehicle yard |
| 46 | construction bee | 1 | 2 |  | 1 |  |  | 0 | vehicle yard |
| 11 | worker bee |  |  |  | 1 |  |  | 0 | vehicle yard |
| 88 | unarmed APC | 5 | 10 |  | 2 |  |  | 240 | vehicle yard, trooper center |
| 95 | science vehicle | 8 | 14 |  | 2 |  | 80 | 640 | vehicle yard, sensor lab |
| 37 | ground scout | 3 | 1 |  | 1 |  |  | 128 | vehicle yard |
| 10 | disruptor tank | 15 | 18 |  | 2 |  | 100 | 640 | vehicle yard, disruptor matrix, vehicle yard upgrade |
| 40 | armed APC | 4 | 3 |  | 1 |  | 30 | 240 | vehicle yard |
| 36 | photon artillery | 10 | 16 |  |  | 100 | 80 | 640 | vehicle yard, security centre, vehicle yard upgrade |
| 228 | disruptor battery | 18 | 26 |  | 2 |  | 120 | 640 | advanced hub, defensive weapon, defensive artillery, special weapons |
| 229 | cloaked disruptor tank | 20 | 20 |  | 4 |  | 80 | 640 | advanced hub, defensive weapon, defensive artillery, disruptor matrix |
| 32 | storage facility |  |  |  |  |  |  | 0 | construction yard, storage processor |
| 30 | power generator | 35 | 1 | 30 |  | 850 |  | 0 | construction yard |
| 53 | construction yard |  | 12 | 89 | 50 | 30 |  | 0 |  |
| 31 | vehicle yard |  | 10 | 90 | 5 |  |  | 0 | construction yard |
| 51 | storage processor |  | 4 | 30 | 3 |  |  | 0 | construction yard |
| 52 | mining station |  | 14 | 80 | 50 |  | 140 | 0 | storage processor, construction yard |
| 50 | livestock farm |  | 18 | 75 | 38 |  |  | 0 | improved yard, landing pad, accomodation block |
| 73 | landing pad |  | 6 | 32 | 2 |  |  | 0 | improved yard |
| 127 | science block |  | 4 | 53 | 32 | 120 |  | 0 | construction yard |
| 124 | sickbay |  | 8 | 48 | 26 |  |  | 0 | construction yard |
| 68 | accomodation block |  | 16 | 60 | 40 |  |  | 0 | improved yard, landing pad |
| 75 | security centre |  | 10 | 100 | 100 | 120 |  | 0 | improved yard |
| 54 | defensive weapon |  |  | 92 | 12 |  | 500 | 0 | improved yard, shield generator, disruptor matrix, security centre |
| 29 | shield generator | 20 | 2 | 20 |  |  | 800 | 0 | improved yard, security centre, shield buffer |
| 82 | defensive artillery |  |  | 93 | 11 | 250 | 500 | 0 | advanced yard, shield generator, tracking systems |
| 172 | advanced power generator | 50 | 2 | 37 |  | 870 |  | 0 | advanced yard |
| 189 | advanced mining station |  | 18 | 92 | 75 |  | 1600 | 0 | advanced yard |
| 207 | CFD |  | 6 | 110 | 52 | 300 |  | 0 |  |

### Romulan

| id | type | dilithium | polycomp | thermoconc | trans al | nitrium | duranium | build value | needs |
|---|---|---|---|---|---|---|---|---|---|
| 90 | cargo bee | 1 | 2 |  | 1 |  |  | 128 | vehicle yard |
| 91 | construction bee | 1 | 2 |  | 1 |  |  | 0 | vehicle yard |
| 92 | worker bee |  |  |  | 1 |  |  | 0 | vehicle yard |
| 89 | unarmed apc | 5 | 10 |  | 2 |  |  | 0 | vehicle yard, infiltration lab |
| 63 | science vehicle | 8 | 14 |  | 2 |  | 80 | 640 | vehicle yard, science block, sensor lab |
| 98 | ground scout | 3 | 1 |  | 1 |  |  | 0 | vehicle yard |
| 148 | tank | 15 | 18 |  | 2 |  | 100 | 640 | vehicle yard, science block, disruptor matrix |
| 64 | armed APC | 5 | 5 |  | 1 |  | 30 | 240 | vehicle yard |
| 87 | photon artillery | 10 | 16 |  |  | 100 | 80 | 640 | vehicle yard upgrade, security centre |
| 35 | cloaked tank | 18 | 18 |  | 2 |  | 120 | 640 | vehicle yard upgrade, energy lab |
| 230 | cloaked apc | 5 | 7 |  | 1 |  | 40 | 240 | advanced hub, defensive weapon, defensive artillery, sensor lab |
| 66 | storage facility |  |  |  |  |  |  | 0 | storage processor |
| 70 | power generator | 35 | 1 | 30 |  | 850 |  | 0 | construction yard |
| 78 | construction yard |  | 12 | 89 | 3 | 500 |  | 0 | hub |
| 71 | vehicle yard |  | 10 | 90 | 2 |  |  | 0 | construction yard |
| 121 | storage processor |  | 4 | 30 | 2 |  |  | 0 | construction yard |
| 65 | mining station |  | 14 | 80 | 4 |  | 140 | 0 | storage processor |
| 182 | clone farm |  | 18 | 75 | 36 |  |  | 0 | yard upgrade, landing pad, accomodation block |
| 72 | landing pad |  | 6 | 32 | 2 |  |  | 0 | yard upgrade |
| 128 | science block |  | 4 | 53 | 22 | 120 |  | 0 | construction yard |
| 125 | sickbay |  | 8 | 48 | 36 |  |  | 0 | construction yard |
| 67 | accomodation block |  | 16 | 60 | 42 |  |  | 0 | yard upgrade, landing pad |
| 74 | security centre |  | 10 | 100 | 27 | 1200 |  | 0 | yard upgrade, improved hub |
| 81 | defensive weapon |  |  | 92 | 12 |  | 500 | 0 | yard upgrade, security centre, shield generator |
| 69 | shield generator | 20 | 2 | 20 |  |  | 800 | 0 | yard upgrade, science block, shield buffer |
| 84 | defensive artillery |  |  | 93 | 11 | 250 | 500 | 0 | yard upgrade, advanced hub, security centre, energy lab |
| 178 | advanced power generator | 50 | 2 | 37 |  | 870 |  | 0 | yard upgrade, advanced hub |
| 173 | advanced mining station |  | 18 | 92 | 7 |  | 1600 | 0 | yard upgrade, advanced hub, storage processor |
| 208 | CFD |  | 6 | 110 | 52 | 300 |  | 0 |  |

Upgrades (all sides alike unless noted): improved hub 4 polycomp, 40
thermoconc, 1000 duranium; advanced hub 50, 30, 20 trans al; improved yard
2, 30, 4 trans al (Klingon and Romulan 2); advanced yard 20, 10, 6 trans al
(Klingon 2; Romulan as the improved one); vehicle yard upgrades 1, 50, 3;
science labs 10 polycomp, 15 thermoconc, 7 trans al (Klingon 4, Romulan
5); sickbay wards 2, 25, 6 (4, 5); processor plants 2, 12, 10 (Klingon and
Romulan 2 trans al).

## Vehicles and turrets

Hit points as the game sets them; for the human player's side in a single
player game they are raised (Federation x1.2, Romulan x1.3, Klingon x1),
and a switch doubles them. Weapons hit at once (no travelling shot):
**burst** weapons fire their shots (4) and reload; each shot does the
damage shown times (the best crew weapons skill + 1). **Single** weapons
(Klingon) fire one shot a reload, damage not divided; **artillery** lobs a
shell. Range in world units (a tile is 8192, a block 16384): 20000 is 1.2
blocks. Reload in simulation ticks. Traverse is the turret's yaw limits
(180,180 turns all round), elevation up and down, in degrees. Pace is the
game's path steps a tick (how long a step is wasn't found, so no world
speed yet): scouts, bees and Klingon tanks 0.125, most others 0.0833. Fog:
the size of the area a unit reveals (classes 3 and 4: 5 blocks across; 5:
7 blocks). Scan: sensor range.

| id | side | type | hp | weapon: damage / range / reload ticks / shots / traverse / elevation | pace | fog | scan | build value |
|---|---|---|---|---|---|---|---|---|---|
| 8 | F | phaser tank | 30 | burst 6 / 20000 / 40 / 4 / 180,180 / 20,-45 | 0.08333 | 3 | - | 640 |
| 55 | F | unarmed APC | 30 | - | 0.08333 | 3 | - | 0 |
| 56 | F | armed APC | 50 | burst 6 / 20000 / 40 / 4 / 180,180 / 10,-45 | 0.08333 | 3 | - | 240 |
| 12 | F | worker bee | 5 | - | 0.16667 | - | - | 0 |
| 13 | F | cargo bee | 5 | - | 0.125 | - | - | 128 |
| 14 | F | construction bee | 5 | - | 0.125 | - | - | 0 |
| 97 | F | ground scout | 30 | - | 0.125 | 5 | 60000 | 0 |
| 47 | F | photon artillery | 50 | artillery 80 / 20000 / 40 / 1 / 180,180 / 0,-90 | 0.08333 | 3 | - | 640 |
| 9 | F | advanced photon artillery | 60 | artillery 80 / 40000 / 20 / 1 / 180,180 / 10,-90 | 0.08333 | 3 | - | 640 |
| 80 | F | defensive weapon | 200 | burst 26 / 20000 / 40 / 4 / 180,180 / 5,-70 | - | - | - | 0 |
| 62 | F | science vehicle | 30 | - | 0.08333 | 4 | 18000 | 640 |
| 83 | F | defensive artillery | 200 | artillery 80 / 20000 / 40 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 154 | F | mobile shield generator | 90 | - | 0.08333 | 2 | shield | 640 |
| 88 | K | unarmed APC | 30 | - | 0.08333 | 3 | - | 240 |
| 10 | K | disruptor tank | 60 | single 40 / 20000 / 40 / 1 / 180,180 / 5,-60 | 0.125 | 3 | - | 640 |
| 229 | K | cloaked disruptor tank | 60 | single 40 / 20000 / 40 / 1 / 180,180 / 10,-60 | 0.125 | 3 | - | 640 |
| 228 | K | disruptor battery | 90 | single 80 / 20000 / 20 / 1 / 180,180 / 10,-60 | 0.125 | 3 | - | 640 |
| 11 | K | worker bee | 5 | - | 0.16667 | - | - | 0 |
| 36 | K | photon artillery | 50 | artillery 80 / 20000 / 40 / 1 / 180,180 / 10,-90 | 0.125 | 3 | - | 640 |
| 37 | K | ground scout | 30 | - | 0.125 | 5 | 60000 | 128 |
| 40 | K | armed APC | 30 | single 20 / 20000 / 40 / 1 / 90,-90 / 15,-50 | 0.08333 | 3 | - | 240 |
| 46 | K | construction bee | 5 | - | 0.125 | - | - | 0 |
| 61 | K | cargo bee | 5 | - | 0.125 | - | - | 0 |
| 54 | K | defensive weapon | 200 | single 80 / 20000 / 40 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 82 | K | defensive artillery | 200 | artillery 80 / 20000 / 40 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 95 | K | science vehicle | 30 | - | 0.08333 | 4 | 18000 | 640 |
| 84 | R | defensive artillery | 200 | artillery 80 / 20000 / 40 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 89 | R | unarmed apc | 30 | - | 0.08333 | 3 | - | 0 |
| 35 | R | cloaked tank | 60 | burst 13 / 20000 / 40 / 4 / 180,180 / 5,-75 | 0.08333 | 3 | - | 640 |
| 98 | R | ground scout | 30 | - | 0.125 | 5 | 60000 | 0 |
| 87 | R | photon artillery | 60 | artillery 80 / 20000 / 20 / 1 / 180,180 / 0,-90 | 0.08333 | 3 | - | 640 |
| 81 | R | defensive weapon | 200 | burst 26 / 20000 / 40 / 4 / 180,180 / 5,-90 | - | - | - | 0 |
| 64 | R | armed APC | 50 | burst 6 / 20000 / 40 / 4 / 60,-60 / 15,-45 | 0.08333 | 3 | - | 240 |
| 230 | R | cloaked apc | 50 | burst 6 / 20000 / 40 / 4 / 60,-60 / 15,-45 | 0.08333 | 3 | - | 240 |
| 90 | R | cargo bee | 5 | - | 0.125 | - | - | 128 |
| 91 | R | construction bee | 5 | - | 0.125 | - | - | 0 |
| 92 | R | worker bee | 5 | - | 0.16667 | - | - | 0 |
| 63 | R | science vehicle | 30 | - | 0.08333 | 4 | 18000 | 640 |
| 148 | R | tank | 60 | burst 13 / 20000 / 40 / 4 / 180,180 / 30,-60 | 0.08333 | 3 | - | 640 |

Cloaking units (Romulan cloaked tank 35 and APC 230, Klingon cloaked
disruptor tank 229) have no cloak figure: they are given a different
behaviour when ordered, and the same combat figures as the uncloaked ones.
Vehicles repair themselves, 1 hit point every 6400 / max hp ticks (see
[Repairs](#repairs)).

## Buildings

Power buffer: how much a building stores and how fast it may refill it a
tick; use: what it draws from its buffer each tick; output: a generator's
supply a tick at full efficiency. Staff is the number of crew a building
needs to work; housing the crew it holds (the hub 150, accommodation 200).

| id | side | type | hp | power buffer / intake a tick | use a tick | output a tick | staff, housing | scan |
|---|---|---|---|---|---|---|---|---|
| 16 | F | security centre | 400 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 21 | F | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 22 | F | power generator | 200 | - | - | 500 | - | 2500 |
| 23 | F | colony hub | 600 | - | 0 | 500 | 0, 150 | 20000 |
| 24 | F | vehicle yard | 400 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 77 | F | construction yard | 400 | 34000 / 550 | 85 | - | 1, 0 | 5000 |
| 26 | F | storage processor | 400 | 24000 / 300 | 60 | - | 1, 0 | 5000 |
| 122 | F | storage facility | 600 | 44000 / 425 | 340 | - | 1, 0 | 5000 |
| 27 | F | landing pad | 200 | 44000 / 300 | 60 | - | - | 5000 |
| 44 | F | accomodation block | 400 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 45 | F | mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 2500 |
| 163 | F | advanced mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 1666 |
| 80 | F | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 83 | F | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 123 | F | sickbay | 200 | 44000 / 550 | 440 | - | 1, 0 | 5000 |
| 126 | F | science block | 200 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 158 | F | advanced power generator | 200 | - | - | 1500 | - | 5000 |
| 171 | F | hydroponics farm | 400 | 34000 / 425 | 340 | - | 1, 0 | 5000 |
| 118 | F | CFD | 30 | - | 0 | - | - | - |
| 75 | K | security centre | 400 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 73 | K | landing pad | 200 | 44000 / 300 | 60 | - | - | 5000 |
| 28 | K | colony hub | 600 | - | 0 | 500 | 0, 150 | 20000 |
| 29 | K | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 30 | K | power generator | 200 | - | - | 500 | - | 2500 |
| 172 | K | advanced power generator | 200 | - | - | 1500 | - | 5000 |
| 31 | K | vehicle yard | 400 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 51 | K | storage processor | 600 | 44000 / 425 | 85 | - | 1, 0 | 5000 |
| 32 | K | storage facility | 600 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 68 | K | accomodation block | 400 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 50 | K | livestock farm | 400 | 34000 / 425 | 60 | - | 1, 0 | 5000 |
| 52 | K | mining station | 200 | 44000 / 550 | 110 | - | 1, 0 | 2500 |
| 189 | K | advanced mining station | 200 | 44000 / 550 | 110 | - | 1, 0 | 1666 |
| 53 | K | construction yard | 400 | 44000 / 550 | 110 | - | 1, 0 | 5000 |
| 54 | K | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 82 | K | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 124 | K | sickbay | 400 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 127 | K | science block | 200 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 207 | K | CFD | 30 | - | 0 | - | - | - |
| 84 | R | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 74 | R | security centre | 400 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 69 | R | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 71 | R | vehicle yard | 400 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 72 | R | landing pad | 200 | 44000 / 300 | 60 | - | - | 5000 |
| 43 | R | colony hub | 600 | - | 0 | 500 | 0, 150 | 20000 |
| 81 | R | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 66 | R | storage facility | 600 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 121 | R | storage processor | 400 | 44000 / 425 | 85 | - | 1, 0 | 5000 |
| 78 | R | construction yard | 400 | 34000 / 425 | 85 | - | 1, 0 | 5000 |
| 70 | R | power generator | 200 | - | - | 1000 | - | 2500 |
| 178 | R | advanced power generator | 200 | - | - | 3000 | - | 5000 |
| 67 | R | accomodation block | 400 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 182 | R | clone farm | 400 | 34000 / 425 | 340 | - | 1, 0 | 5000 |
| 65 | R | mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 2500 |
| 173 | R | advanced mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 1666 |
| 125 | R | sickbay | 200 | 44000 / 425 | 340 | - | 1, 0 | 5000 |
| 128 | R | science block | 200 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 208 | R | CFD | 30 | - | 0 | - | - | - |

## The economy

- **Mining.** A map cell's 16-bit value holds its ore kind (bits 12-15) and
  amount (bits 0-11, the 50-300 seen in the maps). A mining station works
  the 7 x 7 blocks round its own, taking from the nearest ore cell (ring by
  ring); each take uses one unit of the cell's amount and gives a tonne of
  raw ore. A cell at 0 turns into plain ground. A mine fills 40 t loads and
  waits for the bees to carry them (cargo bees carry two, worker bees one);
  with 4 loads waiting, or no crew, it waits 24 s before trying again. A
  counter shared by all the colony's mines makes every mine yield all six
  ores: in each 8 takes, 2 of the cell's own kind and 1 of each kind (so 3
  to 1). The rate the processor receives therefore depends on the bees;
  the training missions measured about 2 t a second per mine.
- **Refining.** The resource processor turns to one of its 16 store slots
  a tick and refines up to a quarter tonne of that ore, one for one, into
  its material: **dilithium into dilithium, talgonite into polycomp,
  silicon into thermoconc, kelbonite into trans al, magnesite into
  nitrium, dolamide into duranium**. A processor plant for a material
  doubles it. About 0.31 t a second of each at most - shown as 0.31 a
  second for the first four and 3.1 for nitrium and duranium (counted in
  hundreds), as C5 measured.
- **Stock.** The sum of every store the side has. The default starting
  stock is 500 / 1000 / 2300 / 500 / 4000 / 5000 (80 of the missions);
  exceptions: F12, K12, R12 800 / 1750 / 2300 / 1000 / 24000 / 25000; J7
  150 / 1000 / 2300 / 200 / 4000 / 10000; R2 dilithium 100; R8 polycomp and
  thermoconc only; the later tutorials less dilithium and nitrium.
- **Construction.** The full cost is checked and taken when the order is
  given; short, nothing is spent and the colony hears it's delayed. A
  placed building becomes a site with room for 4 construction bees; it is
  finished when the bees have finished its model's four corner sections.
  How fast a bee works wasn't traced, so build times stay as measured.
  Neither power nor crew enters into it.
- **Vehicles.** The vehicle yard's work for a vehicle is its build value x
  300 power units; each tick it draws up to 300 from its buffer times its
  efficiency. It stops below 0.2 efficiency and restarts above 0.35. It
  can't build while the colony is at its capacity.
- **Power.** Each side has a pool with a reserve of 640000 (the panel's
  RESERVE % is reserve / 6400). Each tick generators add their output x
  efficiency; buildings ask to refill their buffers (at most their intake)
  in four priority classes; DEMAND MET is what was granted over what was
  asked, CONSUMPTION what was drawn over what was produced. Outputs: hub
  500, Federation and Klingon generator 500, Romulan 1000; advanced 1500
  and 3000. The LOW / MED labels are fixed per building type. The colony is
  warned when less than three quarters of its demand is met.
- **Efficiency.** 0.4 x (health + buffer fill + the best officer skill for
  the building's kind) x the share of its staff it has; generators, hubs,
  shield generators, landing pads, accommodation and turrets always count
  as fully staffed. A whole, powered, staffed generator runs at 1.2 (the
  panel's 120 %); a hub without an officer at 0.8.
- **Colony size and capacity.** COLONY SIZE a / b: a counts the side's
  buildings and vehicles (not crew); b starts at 50 (a mission may set
  another: C1's 60) and never passes 90. Accommodation adds 5, a farm
  (hydroponics, livestock, clone) 10, each sickbay ward 5; nothing else
  (the landing pad, upgrades). That gives F1's 50, 55, 65, 80, 90.
- **Crew.** Each building has places for its staff and occupants; it works
  only with its staff in. The hub holds the colony's pool (150).

## Missions

48 missions are registered, each with its map (an archive and its map's
place in it). They reproduce every campaign map in [level.md](level.md),
and add: the training missions C1 and C4-C8 on `coverd3` (in `alpwrld.cat`),
C9 on `demo1`, C10 on `coverd1`, C11 on `f1`; also two cover-disk demos,
head-to-head and test levels.

Each mission has seven side slots: 0 Federation, 1 Klingon, 2 Romulan,
3-5 the natives (which race is where varies), 6 the whole mission. A side
holds its script objects, its **starting placements**, AI orders, its
objectives and fail criteria (by the mission's `.stb` string ids, hidden
ones flagged), research topics, starting stock, its stance towards each
other side, and named officers.

### Starting colonies

Each side's placements are its starting colony: a type and a map position
(block column and row, and which quarter of the block). 21 of the 23
starting mining stations sit on ore deposits read that way. Vehicles
placed on the hub start inside it (its panel's bee and transport counts).
F1's Federation hub is at 438:218, four blocks from the recorded Tricorder
reading of 407:209; the sites guessed from recordings ([colony.md](colony.md))
were 1-19 blocks off. Reluva places every mission's colony and vehicles
from these (`start_data.cpp`).

### The script engine

A mission's logic is a graph of named objects, each of one of 49 kinds
(34 used). A message is a plain string sent from one object to its
targets. The main kinds:

| Kind | What it does |
|---|---|
| Timer | Counts down its seconds, then sends its message to its targets; a "Random Event" timer instead sends to one of its eight targets at random and starts again. |
| Relay | Sends its own message when it has received N messages (N = 0: every time). Every mail line, objective counter and mission pass/fail director is one. |
| Distributor, filter, switch | Pass messages on: to all targets; only the first N; only while switched on. |
| Count monitors | Fire when the side has exactly / at least / at most N buildings or units of a type or group (N = 0 with "at most": all destroyed). |
| Capture monitor | Fires when a type is held by a side; can toggle, or require it held for a time (J1's temple: 60 s). |
| Colony size monitor | Fires when the colony has grown or shrunk to a percentage of its starting size. |
| Cell seen / scanned | Fires when the side has seen, or scanned, a map cell (the "approach" and scan events). |
| Group condition | The scan / capture sequence: a named group destroyed, captured or scanned (weak or strong, science / medical / engineering) for a set time; sends start, lost, success, "beaten to it" and futile messages to its targets. |
| Area detectors | Fire when units of a side or a group are inside or outside a rectangle. |
| Spawners and effects | Create attack groups, beam a group off the map, damage a group, move a unit along waypoints, change a side's stance, give, take or top up resources, spread a disease, meteor storms, and "build and hold for 10 minutes" (the CFD objectives of F12, K12, R12). |

Built-in receivers per side take objectives (by name, "Primary Objective
n"), fail criteria, research and the mail: a mail line is sent whole, an
optional speech file name, then a three-digit officer number, the heading
and the text. A mission passes when its scripts count its objectives and
then send `pass` (10 s message, 20 s pass); any fail criterion fails it the
same way.

Compared with Reluva's guessed scripts ([mission-scripts.md](mission-scripts.md)),
the objectives and fail criteria have the right string ids, but the logic
often differs: F1's temple scan starts two clocks (the Taubat turn hostile
after 325 s; the mission fails at 1200 s), not one 300 s fail; F1's
Klingons attack on a schedule (100, 500, 800 and 1100 s, and a random
assault every 300 s) and build to a timetable. The scripts are to be
rewritten from the executable's graphs.

### Sides and combat

Every slot of every mission, not only the player's, is in the
executable: 3560 placements (the briefing's phase-2 objects left out),
157 sides with their stances, and 1756 AI orders. Reluva carries them in
`ai_data.cpp` (generated: type ids, figures, positions and the order
names the scripts use - no text).

**The natives' slots.** Which race holds slots 3-5 varies by mission; it
shows in what they place (by type id). Taubat: temple 110 (and 119 in
F1's slot 3), tank 111, artillery 112, APC 113, house 114, hangar
(workshop) 115, gun 316. Metar: units 48, 99-101, 105, 106, 129, 203-206,
hub 102, building 103, portal 201, starships 223 and 262, turret 317, and
copies of a captured colony's buildings (J4: 209-214); 119 in a Metar slot
is the anomaly device. Hubrin: unit 108, fortress 109. Orion: 117, 264
(R2 and C11 put Orion tanks in the Taubat slot). K2's rebel Taubat (slot
4) have their own ids, 138-143. R1's rival Romulans (slot 4): tank 149,
hub 150, defensive weapon 153. Their models are matched by name from
`foractor.cat` / `forbuild.cat` (`tt` temple, `tw` workshop, `th` house,
`tgp` gun platform, `ta` artillery, `tapc`, `tsg` static gun; Metar `mch`,
`mport`, `mss`, `mad`, the units `mb`, `md`, `mj`, `mn`, `mr`, `my` and
their second kinds; Hubrin `h`, `hf`; Orion `odv`). Which Metar unit is
which kind is a guess from their weapons (two kinds of each share the
figures).

**Stances.** Each side has seven postures, its own towards each slot: 1
self or ally, 2 neutral, 3 hostile. They are one-sided: in F1 the
Klingons are hostile to everyone, the Federation neutral to the Klingons
and hostile to the Romulans, the Taubat neutral to all. The script's
posture switch sets one side's posture towards one other (F1's Taubat
turn hostile to the Federation 300 s after the temple scan).

**Orders.** An AI side's orders come in named and unnamed forms. Named
ones wait for a script message (start, `Abort`, `Trigger`); unnamed ones
run from the start. Each gives a task, a type, a count, a time and up to
four map positions. The nine constructors take their arguments in
different orders (the record: task, a flag "also for a human side", a
mode, type, count, time, positions, name). The time is mission time: the
order may run once the game's tick counter reaches 16 x its value, not
counted from its message.

| Task | Orders | Reading |
|---|---|---|
| 1, 2 | 293 unnamed, 76 named | patrol round the positions (F3's Klingon tanks, R8's "prepositioners") |
| 4 | 102 named (`..._Protection_...`), 2 unnamed | guard the position |
| 5 | 386 named | assault along the positions |
| 6, 7, 8 | 655 named (most of the random raids) | assaults too, by their names |
| 9 | 221 named, 6 unnamed | build the type at the position (F1's Klingons: construction yard at 60 s, vehicle yard 180, processor 300, accommodation 420, shield 540, science 660), and again whenever it's gone |
| 3 | 17 unnamed | build the type at the position, once |

Every build order names a building. 20 orders have the "also for a human
side" flag; in the campaigns the player's own are R8's four
prepositioners (they move the player's starting units into place), F8's
tractor, F1's briefing patrol and one that never runs.

73 **attack group** script objects create an order's units: the count of
the type round one position ("with a transporter effect"), then start the
order. C9's Orion raids come this way (from 434:301 and 326:281); 66
orders have such a spawn point. Other orders take the side's own units:
F1's Klingons start with their tanks, artillery and scouts parked at the
hub ("units are placed at the hub spot and spread by the game").

**Combat.** The figures are in [Vehicles and turrets](#vehicles-and-turrets)
and [Buildings](#buildings): hit points, burst / single / artillery
weapons with range and reload in ticks. Buildings and the natives'
structures have hit points too (Taubat house 200, hangar and temple 400,
Metar hub 400, Hubrin fortress 400 with three guns, Metar starship 800);
a few natives' types carry several weapons. Not found: how a unit picks
its target, whether a moving unit fires, and whether a burst's shots are
spaced.

### Scans and captures

**Scans.** Ground scouts and science vehicles scan (their playbook offers
science, medical and engineering scans; a science scan is their default
action on another side's unit or building). The range is the type's
sensor range plus the target's size: 60000 for a scout, 18000 for a
science vehicle; out of range, the scanner drives closer. A scan has no
length of its own: every tick the scanner is in range it marks the
target as scanned by its side, with one of six flags (weak or strong
science, medical, engineering), and the marks fade two ticks after it
stops. The mission's **raids** (82 of its group watches, `ai_data.cpp`)
read them: the flag held for their start time (1 to 160 script ticks)
sends the start, held as long again as their hold time (160 = 10 s, 480 =
30 s) the success; a break between sends "lost". So a scan only
succeeds if it is kept up, in range, to the end. A scan is **strong**
when an officer aboard is good enough at it: an officer of the scan's
speciality (science 4, medical 3, engineering 1) counts their skill, a
command officer (7) their skill less a quarter (at least a quarter) for
engineering; the best must reach 0.5. Officers' skills are 0.25-1 (F1:
the doctor 0.5, the others 0.25). Scouts carry no one, so their scans are
weak. On a scan's second tick the side's "scan database" relay sends its
stock reply for the kind (nothing unusual found), once per spell of
scanning. Buildings don't scan, and no AI order scans.

**Captures.** Only armed APCs capture ("Capture building", on another
side's building). The APC first fires on it until it's down to 80 % of
its hit points, then drives to its entrance and sends in an away team of
ten crewmen; 336 ticks (16.8 s) later the building is **held** by the
APC's side: its hit points full, its owner unchanged (the panel says
CAPTURED), and it goes on working. One APC at a time. The hold lasts as
long as the APC keeps the order; another order brings the team out and
the building's hit points back to what they were. If the APC is
destroyed the building stays held. The scripts' capture watches read the
hold (J1's temple: held 60 s).

**Officers.** A side's officers are named, each with a speciality and
skill, posted in a building (the hub, the sickbay, a yard). A vehicle
with room for people (science vehicles, APCs) takes up to 16; an
officer moves to a selected vehicle within 1414 units of where they are.

### Fog of war

The map is a grid of one cell a block, each with six channels holding a
bit for each side (slots 0-5). Every object marks cells round its own
block for its side by its type's **fog class** (an 8 × 8 window; where it
stands inside its block doesn't matter):

| Class | Seen | Scanned | Surveyed | 3rd, 4th | Shield field | Types |
|---|---|---|---|---|---|---|
| 1 | disc 7 | | | | | |
| 2 | disc 7 | | | | 3 × 3 | shield generators |
| 3 | disc 5 | | | | | tanks, APCs, artillery |
| 4 | disc 7 | 3 × 3 | disc 7 | disc 7 | | science vehicles, most buildings, the Klingon hub |
| 5 | disc 7 | | disc 7 | | | scouts, the Federation and Romulan hubs, mines |
| 6, 7 | disc 7 | | | | plus / one block | (none) |
| 8 | | | | | | one native type: on its block, every other side's marks are wiped |

(disc 7: 37 blocks in rows 3, 5, 7, 7, 7, 5, 3 wide; disc 5: 21 blocks.)
The marks mean **covered now**: each tick the cells of one of the 64
residues (x and y modulo 8) are cleared and marked again, so the whole
map is redone every 64 ticks (3.2 s); nothing remembers what was once
explored. Another side's units and buildings are drawn, and shown on the
Tricorder, only on cells the player's side sees; the camera can't be
moved onto a cell it doesn't see. The script's "cell seen" monitor reads
the seen channel, "cell scanned" the scanned one (a science vehicle or a
building next to the cell); the Tricorder's mineral readout reads the
surveyed channel. The shield field channel changes how a building inside
it takes damage. The other two channels' use wasn't found.

### Script effects

The script objects that act on the world, as the game runs them (16
ticks a second, like all script objects):

- **Group damage**: once, every unit of a named group loses a share of
  its full strength: 20, 40, 60 or 80 %, or all of it.
- **Group beam-up**: every unit of the group beamed off the map; the
  object fires when the group is empty.
- **Attack group**: the count of a type created for a side round a point,
  with a transporter effect, and handed to an AI order.
- **Unit path**: a unit (a Bird of Prey, a shuttle, the tractor) sent
  along up to 16 points, stopping at the end or looping.
- **Meteor storm**: whenever the game time divides by its interval (F2:
  16, 24 and 32 ticks: 1, 1.5, 2 s), a meteor falls at a random point of
  its rectangle and does **30** damage to the one object it hits (no
  splash).
- **Area storm**: lightning only: a 2 % chance a tick of a flash (half a
  second, the camera shaken when near) somewhere in its rectangle.
- **Commodity top-up**: every 63 ticks (3.9 s), any material below the
  bundle is made up to it, into the side's stores, free.
- **Thief / donor**: the bundle taken from / given to the side's stock.
- **Disease** (F2): among the crew held by buildings and vehicles (not the
  officers). Each tick one crewman in turn rolls (0-100): while it
  spreads, a well one catches it under 10, an ill one loses health under
  4 (dying at none: rare); once the cure starts, an ill one gets better
  under 60, until all are well, when the object fires. The disease
  monitor fires when the infected reach its share of the crew (F2: 1,
  30, 80 %).
- **Build and hold**: a type's count reaching its number starts a clock
  (600 s for the CFD objectives); losing it stops the clock.
- **Area detectors**: every 16 ticks, a side's units of a class inside
  (or none inside) a rectangle; a group detector, the whole group.

### Crew and efficiency

A building's **efficiency** is 0.4 × (health + power + officer) × crew:
- health: by its hit points' share in twentieths: 0-0.5 in tenths up to
  30 %, then 0.55-0.95, whole from 75 %;
- power: its buffer's fill, (4 × average + now) / (4 × capacity), whole
  above 99 %; the average is halved towards the fill at each refill.
  Generators and the hub count it whole;
- officer: the best skill among the officers posted there at the
  building's speciality (1 the yards, 2 security, 3 sickbay, 4 science,
  6 mines and processor, 7 the hub, 8 storage, 9 accommodation); whole
  for a building with none (generators);
- crew: its crew over its crew slots, at most 1; a building with less
  than the crew it requires does nothing. Generators, hubs, pads,
  housing, farms, shields and defences count it whole.

So a healthy generator runs at 1.2 (500 × 1.2 = 600 a tick) and the hub
without a command officer at 0.8 (400). Only the crew term is held to
0-1; the result isn't capped, so 1.2 is the most any building reaches
(checked in the efficiency routine: no clamp after the sum).

**People.** Each race has a roster of **150** named people, the
mission's officers among them. The hub takes everyone still in orbit
into its pool (150 places; accommodation 200 more). A building asks for
its crew slots (1 for most, 3 for the vehicle yard, 8 for accommodation)
when it's put up or finished: the first hub or accommodation block with
that many in its pool sends them all, and they walk over; else a
transporter pad beams them down from orbit; else the request waits until
a pool takes people in. A new accommodation block fills its pool ten at
a time from the hub's pool, then from orbit. Nobody is born and no time
adds people; they die only with what they're in. The vehicle yard won't
build without crew ("not enough crew"). C7's hub pool of 128 is the 150
less the five officers and the crews of the starting buildings and
vehicles.

**Life support.** Everyone on the planet uses 16 units a tick of a
life-support pool (655360); farms add 2048 × their efficiency; the hub
buys the rest back with power before any other building, (what's
missing) / 16.384 / its efficiency. An empty pool was not seen to do
anything.

### Vehicle production

A vehicle yard makes one vehicle at a time. An order takes the cost at
once and needs the colony below its size limit (the yard's button is off
at the limit). The job is the type's **build value × 300** units of
power; each tick a running yard takes up to 300 from its power buffer
and puts that times its **efficiency** into the job. Efficiency is
0.4 × (health + power fill + officer) × crew: health 1 above 75 % of
the hit points, the buffer's fill (1 above 99 %), the best officer of
the yards' speciality (1) posted there (their skill; 0 for none), the
crew share. A yard (re)starts above 0.35 and stops below 0.2. So with a
full buffer and no officer (0.8): build value 640 (tanks, artillery,
science vehicles) takes 800 ticks, 40 s; 240 (armed transports) 15 s;
128 (some scouts and cargo bees) 8 s; 0 (the Federation's scout, bees,
transports) a tick. The training missions timed 40-50 s, about 25 s and
about 8 s, so the yard adds some 8 s of its own (rolling the vehicle
out; not found in the code).

What each side's vehicles need besides the yard, by id: the extended
(146, 183, 177) or specialist yard (147), the security centre, the
science station, the advanced hub (169, 175), defensive weapons and
artillery, and **labs** - add-ons to the science station bought with
materials (10 polycomp, 15 thermoconc and 4-7 trans al each). Federation:
phaser coil lab 294 (phaser tank), energy lab 295 (mobile shield),
sensor lab 296 (science vehicle), away team lab 297 (transport), shield
buffer lab 164. Klingon: special weapons 302, sensor lab 303, trooper
centre 304, shield buffer 184, tracking systems 185, disruptor matrix
186. Romulan: infiltration lab 311, energy lab 310, shield buffer 179,
and 308 (tank) and 309 (cloaked tank), whose names the executable's
tables don't give in order.

### Research

Each side has the mission's **research topics** (`ai_data.cpp`): a name,
a field (1 medical, 3 science; 2 in F8 and 4 in CD1-CD2; the field only
picks the topic's icon), the points it takes (10000-150000) and how many
times its name must reach the side's research database before it opens
(0: open from the start, as J8's; 1 most; 2 J6's biodata). The scripts
send the name when the mission calls for the research. The player starts
an open topic from the Operations panel's Research tab (its button is lit
while any topic is open); nothing starts by itself. Every tick the side's
**1000 research points** are shared out, in whole points, between the
topics running: one alone takes 150000 points in 150 ticks (7.5 s), two
at once go at 500 a tick each. **No building, officer, crew or power
enters into it** (the sickbay and science station named by the messages
don't take part). A finished topic sends its messages to its targets (the
script's next step); the game announces nothing of its own. A topic's
button reads one of three captions: not started, in progress, finished.
Officer training runs in a second queue the same way ("train staff":
150000 points). The science station's labs are add-ons bought with
materials, not research.

### AI colonies

How a side the computer plays builds and makes units:

- **Turns.** Each side's AI commander checks one of its orders a tick, in
  turn, and only for a side that isn't human unless the order's flag says
  otherwise. An order that isn't ready waits for its next turn; there is
  no other retry.
- **Builds** (tasks 3 and 9) use only the type, the time and the first
  position, snapped to the grid; the count is ignored. Ready when the
  time has come and the spot passes the player's own placement test (a
  mining station's needs its deposit); nothing searches round it. It then
  goes through the player's construction routine: a site, its foreman
  asking for four of the side's construction bees. **A side that isn't
  human pays nothing**, and what a building needs isn't checked. Task 3
  fires once (any message cancels it before then); task 9 stays armed, so
  it puts the building up again once it's gone. `Abort` disarms it and
  any other message arms it again; both start armed.
- **Units** for patrols, guards and assaults: the group asks for its
  count of the type, one at a time. The side's field marshal hands over
  an idle unit; failing that, its production ministry passes the request
  to a factory, one per vehicle yard, which makes it as the player's yard
  does (build value x 300 at the yard's efficiency), **free to an AI
  side**. Without a yard the request just waits; only the attack-group
  script objects beam units in. The group sets off once it has them all.
  A human side's pool passes on only requests for worker bees.
- **Nothing else.** No building list, unit production or repairs of the
  AI's own were found; it builds only what its orders say, and rebuilds
  only through task 9. Every side has the same commander, ministries and
  buildings, so power, crew and officers set an AI yard's pace as they do
  the player's. Upgrades are always charged, but no order asks for one.

### Repairs

Every object's repair runs each tick, in battle as well; only a building
still going up and a destroyed object are left out.

- **Vehicles**, and the natives' buildings: a hit point, then a wait of
  trunc(6400 / max hp) / (2 x skill + 1) ticks (at least 1), skill being
  the best engineer aboard (a command officer counts at a quarter less,
  at least a quarter). Whatever its hit points, a vehicle takes about
  6400 ticks (320 s) to mend from nothing, a third of that with a
  skill-1 engineer. No power.
- **Colony buildings** draw on their side's **repair pool** (up to
  2 097 152; not the power pool). Each tick a building holding less than
  16384 asks for the rest, at most 2048, in its priority class (served
  1, 2, 3); each 16384 it holds buys a step of 2 x skill + 1 hit points.
  At the full draw that is a step every 8 ticks. A building that holds
  nothing mends at the vehicles' rate. Efficiency plays no part.
- **Construction yards** fill the pool, 8192 a tick each while standing,
  and don't repair themselves; nothing else fills it. A yard feeds about
  four buildings at the full draw; without one every building mends
  slowly.

| Class | Types |
|---|---|
| 1 | 21 22 23 24 30 31 43 54 69 70 71 75 80 81 82 83 84 118 123 158 171 172 178 207 208 |
| 2 | 27 29 32 44 45 49 52 65 66 72 73 74 122 125 126 127 128 163 173 182 189 |
| 3 | 16 25 26 50 51 67 68 76 121 124 |
| yard | 53 77 78 |

### Shields

A hit on a building, an armed vehicle, an armed APC, a science vehicle
or a scout is lessened: halved (rounded down) when its block is in a
shield field of its own side (fog channel 5), then divided by
4 x skill + 1 for its best security officer (a command officer at a
quarter less), field or not. Nothing drains: the generator's power
buffer isn't touched, and the halving has no limit. Bees, unarmed APCs,
large craft and the tractor and anomaly devices always take the whole
hit, as does another side's unit in the field.

- **Shield generators** (21, 29, 69) pick their field each tick from
  their efficiency: above 0.6 the 3 x 3 blocks round them, above 0.2 a
  plus of five, else their own block. With no officer term (0.4 x
  (health + power fill + 1)) that is 3 x 3 while powered, or while its
  hit points are at least 30 %, and the plus below that.
- **The mobile shield generator** (154) always covers the 3 x 3 blocks
  round itself as it goes; it has no power of its own.
- The fields follow the fog's refresh, a cell every 64 ticks.
- The Shield buffer lab only allows the Federation's shield generator;
  no shield figure was found on vehicles.

### Cloaking

Types 35 (and 272), 229, 230, 200 and 149 carry a cloaking device:

- After 300 ticks (15 s) seen it fades out over 63 ticks; then it's
  cloaked. No power or movement condition.
- To fire it must be seen: cloaked, it starts decloaking (63 ticks,
  seen from the first) and can't fire while fading either way. Once seen
  the count starts again, firing or not, so in a long fight it cycles:
  63 ticks decloaking, 300 firing, 63 cloaking.
- Cloaked, other sides don't draw it or show it on the Tricorder, and
  every automatic target search passes it by (as it does units dead or
  still being built); a hit still counts if something aims at it. No scan
  that finds it was found.
- Its "attack en route" move only turns its turrets home, so it drives
  without fighting and stays cloaked.

### Orders

The playbook's entries and what they give (a group commander for the
selection; each unit staggers its start by 0-11 ticks):

| Menu entry | Order | Behaviour |
|---|---|---|
| Follow and attack enemy (and a plain click on an enemy) | target kind 1 | engages if it can, else paths to a firing position (up to 37 random points, the radius growing 1.25 times each round), firing at others on the way; re-plans after 20 ticks without a shot or about every 7 ticks; chases without limit |
| Assault enemy/location | target kind 2 | brakes and fires at the targets from where it stands; no approach |
| Attack en route | location kind 2 | moves, and every 8-11 ticks each turret picks the nearest hostile, seen, uncloaked target within the unit's weapon range (or its attacker); never stops or chases |
| Move without attacking (and a plain click on the ground) | location kind 1 | the same, except that cloaking types (35, 149, 200, 229, 230) don't fire |
| Protect from location (limited range) | location kind 4 | drives to the spot and holds, firing within weapon range; every 8 ticks, if anything of the side has a live attacker, chases the nearest, then returns |
| Protect area (mobile shield generator) | target kind 5 | guards the clicked buildings from where it stands; when one is attacked it drives to the attacker, its field with it, and returns; it never fires |
| Return to vehicle rally point | no order | the unit is handed to the side's field marshal |

Units with nothing to do fire from where they stand at hostiles within
weapon range, retaliate when they can reach the attacker, and neither
chase nor return; after an order a group holds like this for 2000 ticks
at a time.

**The rally point.** New vehicles and those sent back go to the side's
field marshal, which gives them to an AI request that wants them, else
to a group that takes them (a lager takes up to 16 armed units), else
starts a new lager. A lager stands at the colony hub, snapped to the
corner of its block, with 16 parking places 4096 apart from 2560 inside
that corner (4 x 4). There is one for the side, not one per yard, and
nothing found moves it. A unit within 4096 of its place stands and
fires; one that can't get there tries again after 320-816 ticks.

### Group counts

Each object type has a class mask: 0x2 small buildings (generators,
shields, storage, pads, science blocks, the Taubat house), 0x4 large ones
(hubs, yards, accommodation, sickbay, farms, security, the Taubat hangar
and temple), 0x8 mines, 0x10 turrets, 0x100 armed vehicles, 0x40000
armed APCs, 0x400000 science vehicles, 0x4000000 scouts, 0x20000 large
craft. Each side keeps a row per type and up to 16 rows per class mask:
completed, being built, and held by each side. The count monitors (kinds
2, 3, 48 and 49) read the class rows, so their "group ids" 2, 4 and 256
are the small buildings, the large buildings and the armed vehicles.
Kinds 2, 48 and 49 count completed objects not held by another side
(so "no small buildings left" also comes true once the rest are
captured); kind 3 adds the held ones. Sites and vehicles in production
count in neither.

**Colony size** (kinds 8 and 9): small + 3 x large, completed, not held
(kind 9 adds those held by others), against its value on the first tick;
it works for any side, natives included. **Colony capture** (kind 10):
the holder's held small + 3 x large x 100 over the owner's size on the
first tick (J1: the Taubat 22, so 30 % is 7: three hangars, or two and a
house).

### Rival watches

The computer's sides never capture buildings or scan things: those
orders come only from the player's playbook (the AI's own orders attack
a side). So the other sides' raid watches (kind 37) succeed only when
the group is destroyed (condition 1, by anyone) or their own buildings
and vehicles mark its block scanned (condition 3). A watch's success
sends its rivals Go_Futile: one under way reports "beaten to it"
(target 1); one idle or still in its start delay turns futile and later
reports "futile" (target 6) instead of success.

## Still open

- How long a path step is (so units' real speeds), and how fast
  construction bees build.
- How the 50 ms step and the scripts' sixteenths relate.
- How fast bees carry ore from the mines.
- Placement flags, a third placement phase, the AI order task codes 1,
  2 and 6-8 (read from the orders' names), and the order modes.
- Which of the posture bits the scan and capture menus test is hostile
  and which neutral; how the player moves officers (the gesture).
- How the AI picks targets; how far apart the placement test keeps
  buildings.
- Several unnamed type ids (natives' units and buildings, effects).
