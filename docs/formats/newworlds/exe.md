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
- The mission scripts count time in **script seconds of 16 ticks**: a
  timer of N runs 16N ticks, 0.8 N s at normal speed. F1's recording fits
  it to the second (its scout timer of 10 shows 8 s in, its first attack's
  100 after 80 s), so does K1's pass timer. The AI orders' delays and the
  briefing steps' lengths use the same unit; the raids' and detectors'
  delays are raw ticks. A slow machine stretches all of it (the ticks are
  game ticks, not wall time).
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
vehicle yard's work: its value x 300 power units, taken 300 a tick at
its efficiency. Some tutorial readings were misread digits in the small font:
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

Hit points and weapon figures as the game plays them: each type's
constructor reads them from a table the game rewrites once at start-up
(about 300 of its values change: the phaser tank's hit points 30 to 70
and its burst 20 to 30, the Federation hub 600 to 1500, the bees 5 to a
million), so the tables here give the rewritten figures, not the ones
stored in the file. For the human player's side in a single player game
hit points are raised again (Federation x1.2, Romulan x1.3, Klingon x1,
truncated), for every object of its race as it is made, built later too;
the setup's Cadet box doubles them instead. That is the whole of the
difficulty: there is no difficulty setting (the "novice / average /
expert" of the readme are the races), and nothing else changes stock,
size, enemies or speeds. The
Metar starship (262) carries five turrets of its own: a single-shot gun
and four bursts, each 120 (the bursts 40 a pulse), range 45000, reload
20; Reluva fires them as one gun of five.
**Burst** weapons fire 4 pulses, one a tick, each a third of the type's
figure, cut down (the table gives a pulse's damage), and reload from the tick after the last:
with a reload of 40 a burst every 44 ticks. **Single** weapons (Klingon)
fire one shot a reload, damage not divided. Both multiply a shot's
damage by (the shooter's best security officer's skill + 1, a command
officer counting a quarter less, at least a quarter) and cut it down;
vehicles start with no officers, so it's normally 1. **Artillery** lobs a
shell (speed 350 a tick, gravity 3 a tick squared; the high arc when the
line of sight is blocked), with no multiplier and no splash; on flat
ground it can't reach past about 36850 units. Every shot is a real
projectile (bolts 5400 units a tick, beam pulses a beam's length a tick)
that hits the first object of another side its path crosses, or misses
into the ground (see Aiming and Hits under Sides and combat). A fire takes 300 units from the shooter's power buffer
(each burst pulse, and the call that starts the reload, so 1500 a burst),
but only buildings have a real buffer: every vehicle (and the natives'
guns) has an empty stand-in that always says yes and takes nothing. So
only the empires' defensive weapons and artillery can fall silent, when
their side's power can't refill them (each side, the computer's too,
runs the same power economy; a turret's buffer is 64000, refilled up to
550 a tick, using 160). Range in world units (a tile is 8192, a block 16384): 20000 is 1.2
blocks. Reload in simulation ticks. Traverse is the turret's yaw limits
(180,180 turns all round), elevation up and down, in degrees. Pace is the
game's path steps a tick (how long a step is wasn't found, so no world
speed yet): scouts, bees and Klingon tanks 0.125, most others 0.0833. Fog:
the size of the area a unit reveals (classes 3 and 4: 5 blocks across; 5:
7 blocks). Scan: sensor range.

| id | side | type | hp | weapon: damage / range / reload ticks / shots / traverse / elevation | pace | fog | scan | build value |
|---|---|---|---|---|---|---|---|---|---|
| 8 | F | phaser tank | 70 | burst 10 / 20000 / 40 / 4 / 180,180 / 20,-45 | 0.08333 | 3 | - | 640 |
| 55 | F | unarmed APC | 30 | - | 0.08333 | 3 | - | 0 |
| 56 | F | armed APC | 50 | burst 6 / 18000 / 50 / 4 / 180,180 / 10,-45 | 0.08333 | 3 | - | 240 |
| 12 | F | worker bee | 1000000 | - | 0.16667 | - | - | 0 |
| 13 | F | cargo bee | 1000000 | - | 0.125 | - | - | 128 |
| 14 | F | construction bee | 1000000 | - | 0.125 | - | - | 0 |
| 97 | F | ground scout | 30 | - | 0.125 | 5 | 60000 | 0 |
| 47 | F | photon artillery | 60 | artillery 55 / 38000 / 100 / 1 / 180,180 / 0,-90 | 0.08333 | 3 | - | 640 |
| 9 | F | advanced photon artillery | 70 | artillery 85 / 45000 / 140 / 1 / 180,180 / 10,-90 | 0.08333 | 3 | - | 640 |
| 80 | F | defensive weapon | 200 | burst 13 / 30000 / 40 / 4 / 180,180 / 5,-70 | - | - | - | 0 |
| 62 | F | science vehicle | 40 | - | 0.08333 | 4 | 18000 | 640 |
| 83 | F | defensive artillery | 200 | artillery 30 / 40000 / 75 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 154 | F | mobile shield generator | 100 | - | 0.08333 | 2 | shield | 640 |
| 88 | K | unarmed APC | 30 | - | 0.08333 | 3 | - | 240 |
| 10 | K | disruptor tank | 70 | single 30 / 20000 / 40 / 1 / 180,180 / 5,-60 | 0.125 | 3 | - | 640 |
| 229 | K | cloaked disruptor tank | 75 | single 30 / 20000 / 40 / 1 / 180,180 / 10,-60 | 0.125 | 3 | - | 640 |
| 228 | K | disruptor battery | 100 | single 60 / 30000 / 80 / 1 / 180,180 / 10,-60 | 0.125 | 3 | - | 640 |
| 11 | K | worker bee | 1000000 | - | 0.16667 | - | - | 0 |
| 36 | K | photon artillery | 60 | artillery 55 / 38000 / 100 / 1 / 180,180 / 10,-90 | 0.125 | 3 | - | 640 |
| 37 | K | ground scout | 30 | - | 0.125 | 5 | 60000 | 128 |
| 40 | K | armed APC | 50 | single 20 / 18000 / 50 / 1 / 90,-90 / 15,-50 | 0.08333 | 3 | - | 240 |
| 46 | K | construction bee | 1000000 | - | 0.125 | - | - | 0 |
| 61 | K | cargo bee | 1000000 | - | 0.125 | - | - | 0 |
| 54 | K | defensive weapon | 200 | single 40 / 30000 / 40 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 82 | K | defensive artillery | 200 | artillery 30 / 40000 / 75 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 95 | K | science vehicle | 30 | - | 0.08333 | 4 | 18000 | 640 |
| 84 | R | defensive artillery | 200 | artillery 30 / 40000 / 75 / 1 / 180,180 / 5,-90 | - | - | - | 0 |
| 89 | R | unarmed apc | 30 | - | 0.08333 | 3 | - | 0 |
| 35 | R | cloaked tank | 75 | burst 10 / 20000 / 40 / 4 / 180,180 / 5,-75 | 0.08333 | 3 | - | 640 |
| 98 | R | ground scout | 30 | - | 0.125 | 5 | 60000 | 0 |
| 87 | R | photon artillery | 60 | artillery 55 / 38000 / 100 / 1 / 180,180 / 0,-90 | 0.08333 | 3 | - | 640 |
| 81 | R | defensive weapon | 200 | burst 11 / 30000 / 40 / 4 / 180,180 / 5,-90 | - | - | - | 0 |
| 64 | R | armed APC | 50 | burst 6 / 18000 / 50 / 4 / 60,-60 / 15,-45 | 0.08333 | 3 | - | 240 |
| 230 | R | cloaked apc | 75 | burst 11 / 20000 / 20 / 4 / 60,-60 / 15,-45 | 0.08333 | 3 | - | 240 |
| 90 | R | cargo bee | 1000000 | - | 0.125 | - | - | 128 |
| 91 | R | construction bee | 1000000 | - | 0.125 | - | - | 0 |
| 92 | R | worker bee | 1000000 | - | 0.16667 | - | - | 0 |
| 63 | R | science vehicle | 30 | - | 0.08333 | 4 | 18000 | 640 |
| 148 | R | tank | 70 | burst 10 / 20000 / 40 / 4 / 180,180 / 30,-60 | 0.08333 | 3 | - | 640 |

Cloaking units (Romulan cloaked tank 35 and APC 230, Klingon cloaked
disruptor tank 229) have no cloak figure: they are given a different
behaviour when ordered, and the same combat figures as the uncloaked ones.
Vehicles repair themselves, 1 hit point every 6400 / max hp ticks (see
[Repairs](#repairs)).

**Speed.** Paths run over 1024-unit cells, eight ways; a unit takes 1/pace
ticks a cell, so its speed is 1024 x pace a tick (diagonals 1448 x pace).
The start-up rewrite replaces the stored paces: scouts 0.3 (6140 units a
second), tanks and armed APCs 0.1667 (3410), artillery and science
vehicles 0.1 (2050), unarmed APCs 0.0833 (1710). Terrain decides only
where a unit can go, not how fast. Crew figures walk straight, 19 a tick.
Turrets pitch relative to the hull within their limits (a target outside
them is never fired on), at 64 units a tick squared (yaw 128). An
object's size to shots grows over its first attached part (its distance
plus that part's radius); a captured building's grows over the capturing
APC, plus 1000. Everything the simulation animates plays a frame a tick
(20 a second): the bees' arms, the crew, building doors (8 frames), the
16-frame destruction effects.

## Buildings

Power buffer: how much a building stores and how fast it may refill it a
tick; use: what it draws from its buffer each tick; output: a generator's
supply a tick at full efficiency. Staff is the number of crew a building
needs to work; housing the crew it holds (the hub 150, accommodation 200).

| id | side | type | hp | power buffer / intake a tick | use a tick | output a tick | staff, housing | scan |
|---|---|---|---|---|---|---|---|---|
| 16 | F | security centre | 500 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 21 | F | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 22 | F | power generator | 200 | - | - | 500 | - | 2500 |
| 23 | F | colony hub | 1500 | - | 0 | 500 | 0, 150 | 20000 |
| 24 | F | vehicle yard | 450 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 77 | F | construction yard | 450 | 34000 / 550 | 85 | - | 1, 0 | 5000 |
| 26 | F | storage processor | 400 | 24000 / 300 | 60 | - | 1, 0 | 5000 |
| 122 | F | storage facility | 1500 | 44000 / 425 | 340 | - | 1, 0 | 5000 |
| 27 | F | landing pad | 250 | 44000 / 300 | 60 | - | - | 5000 |
| 44 | F | accomodation block | 1000 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 45 | F | mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 2500 |
| 163 | F | advanced mining station | 900 | 24000 / 550 | 110 | - | 1, 0 | 1666 |
| 80 | F | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 83 | F | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 123 | F | sickbay | 400 | 44000 / 550 | 440 | - | 1, 0 | 5000 |
| 126 | F | science block | 450 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 158 | F | advanced power generator | 350 | - | - | 1500 | - | 5000 |
| 171 | F | hydroponics farm | 350 | 34000 / 425 | 340 | - | 1, 0 | 5000 |
| 118 | F | CFD | 2500 | - | 0 | - | - | - |
| 75 | K | security centre | 500 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 73 | K | landing pad | 250 | 44000 / 300 | 60 | - | - | 5000 |
| 28 | K | colony hub | 1500 | - | 0 | 500 | 0, 150 | 20000 |
| 29 | K | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 30 | K | power generator | 200 | - | - | 500 | - | 2500 |
| 172 | K | advanced power generator | 350 | - | - | 1500 | - | 5000 |
| 31 | K | vehicle yard | 450 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 51 | K | storage processor | 1500 | 44000 / 425 | 85 | - | 1, 0 | 5000 |
| 32 | K | storage facility | 1500 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 68 | K | accomodation block | 1000 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 50 | K | livestock farm | 350 | 34000 / 425 | 60 | - | 1, 0 | 5000 |
| 52 | K | mining station | 400 | 44000 / 550 | 110 | - | 1, 0 | 2500 |
| 189 | K | advanced mining station | 900 | 44000 / 550 | 110 | - | 1, 0 | 1666 |
| 53 | K | construction yard | 450 | 44000 / 550 | 110 | - | 1, 0 | 5000 |
| 54 | K | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 82 | K | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 124 | K | sickbay | 400 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 127 | K | science block | 400 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 207 | K | CFD | 2500 | - | 0 | - | - | - |
| 84 | R | defensive artillery | 200 | 64000 / 550 | 160 | - | - | - |
| 74 | R | security centre | 500 | 34000 / 425 | 340 | - | 1, 0 | 20000 |
| 69 | R | shield generator | 400 | 64000 / 550 | 640 | - | - | - |
| 71 | R | vehicle yard | 450 | 34000 / 550 | 60 | - | 3, 0 | 5000 |
| 72 | R | landing pad | 250 | 44000 / 300 | 60 | - | - | 5000 |
| 43 | R | colony hub | 1500 | - | 0 | 500 | 0, 150 | 20000 |
| 81 | R | defensive weapon | 200 | 64000 / 550 | 160 | - | - | - |
| 66 | R | storage facility | 1500 | 24000 / 300 | 240 | - | 1, 0 | 5000 |
| 121 | R | storage processor | 400 | 44000 / 425 | 85 | - | 1, 0 | 5000 |
| 78 | R | construction yard | 450 | 34000 / 425 | 85 | - | 1, 0 | 5000 |
| 70 | R | power generator | 200 | - | - | 1000 | - | 2500 |
| 178 | R | advanced power generator | 350 | - | - | 3000 | - | 5000 |
| 67 | R | accomodation block | 1000 | 34000 / 300 | 240 | - | 8, 200 | 5000 |
| 182 | R | clone farm | 350 | 34000 / 425 | 340 | - | 1, 0 | 5000 |
| 65 | R | mining station | 400 | 24000 / 550 | 110 | - | 1, 0 | 2500 |
| 173 | R | advanced mining station | 900 | 24000 / 550 | 110 | - | 1, 0 | 1666 |
| 125 | R | sickbay | 350 | 44000 / 425 | 340 | - | 1, 0 | 5000 |
| 128 | R | science block | 450 | 34000 / 550 | 440 | - | 1, 0 | 10000 |
| 208 | R | CFD | 2500 | - | 0 | - | - | - |

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
- **Stock.** What the side's storage facilities hold between them (see
  [Stores](#stores)). The default starting
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
  efficiency, once built (the reserve full, the rest is lost); buildings
  ask to refill their buffers (at most their intake) in four priority
  classes, each getting what's left after the one before: 0 the hub's
  life support, 1 vehicle yards, landing pads, sickbays, shield generators
  and defences, 2 the security center and the Federation's storage
  facility (the Klingons' and Romulans' processor), 3 the rest. The
  shares apply on the next tick. A shortfall only drains the buffers,
  so lowers efficiency; building isn't held up. DEMAND MET is what was granted over what was
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
placed on the hub start inside it (its panel's bee and transport counts;
see below).
F1's Federation hub is at 438:218, four blocks from the recorded Tricorder
reading of 407:209; the sites guessed from recordings ([colony.md](colony.md))
were 1-19 blocks off. Reluva places every mission's colony and vehicles
from these (`start_data.cpp`).

Only buildings keep their record's position. The set-up places each
record in the file's order, then hands the vehicles on:

- **Those that fight, scout or scan** (tanks, artillery, armed APCs,
  scouts, science vehicles, mobile shield generators) join the side's
  rally group: up to 16, on its 4 × 4 places in the side's hub's block
  (2560 + 4096 n units in from the block's corner each way), the first
  free one each, placed there at once. The record's position doesn't
  count. A 17th starts at its record and drives in. With no hub (or the
  hub listed after them) the group forms in the block at the map's middle.
- **Bees and unarmed APCs** go inside the hub (if its crew is met);
  without one they stay at their record's position.
- A record's label only names the object.

So spots several sides share in the data (C1's 322:338 for the Klingon
and Romulan tanks, the Klingon bees at another map's hub, 424:71 in
D1-E3, R9's 352:320) are never where those units start.

### Building placement

One test decides where a building can go, for the player's hologram
(green or red) and the AI's build orders alike. Each type has a
footprint, snapped to its middle:

- **a block** (16384 across): hubs, vehicle yards, the Federation and
  Romulan construction yards, accommodation blocks, sickbays, security
  centres (but the Romulan one), the hydroponics farm, the Romulan
  storage processor and science block;
- **a tile** (8192): the Klingon construction yard, storage processors and
  facilities, landing pads, the Federation and Klingon science blocks,
  advanced power generators, the livestock and clone farms, the Romulan
  security centre;
- **a quarter of a tile** (4096): every defensive weapon and artillery,
  shield generators, plain power generators;
- mining stations take a block of their own kind (below).

A spot must be inside the map and not in its outermost ring of blocks; a
block or tile footprint needs all four tiles of its block to be building
ground (a quarter needs nothing of the ground, so turrets go on slopes
or water); the side must see the block now (there is no build radius and
no distance from the hub: anywhere it sees); its part of the block must
be free, and for a tile or quarter the block no other side's; and no
unit may stand in a box round the spot (half the footprint and 512 more
each way, the next block too if it reaches over). Buildings may touch
edge to edge. Ground comes from the map: each tile record's bytes 15-17
mark the colonies' pad ground (model 32), natural building ground
(37-40) and open ground (6-8, 33-36), the same on every map; no slope or
height figure is used. A mining station needs a whole empty block of
open ground and none of building ground; for the player (not the AI)
the block must also hold ore.

An AI build order takes the middle of its waypoint's tile, tests it the
same way each time its turn comes, and builds once it passes; it never
tries another spot and never gives up. The buildings a mission places at
the start skip the test but are snapped to their footprint's middle
(F3's Klingon hub, given as 240:308, stands at 244:308). So F3's Klingon
construction yard goes up at 238:310, against the hub's edge, but its
defensive artillery, ordered inside the hub's own block, never does
while the hub stands; J2's Klingon defensive artillery is ordered onto
the construction yard's tile and waits for as long as the yard stands.

Reluva follows this (`placement.h`): the occupancy is read from what
stands, sites included (a building's cells free once it's gone; the
game's freeing wasn't traced), and units' boxes are their positions.

**No turning.** Every object has a heading, but the player's builds
always take 0 and no key turns them; of the missions' 3868 placements all
face 0 but the wrecked USS Explorer's two halves (134, 135), at half a
turn. Each moving unit keeps a box 512 each way round it, whatever its
type: in a building's way if inside the footprint's half plus 512.

### Transports

The unarmed APC (Federation 55, Klingon 88, Romulan 89) is the colony's
crew shuttle, not a vehicle the player drives. It lives in the hub (off
the map: nothing can see, click or target it there; the A key selects
only armed APCs) and comes out only when the colony's vehicle pool is
asked for one, by:

- a building whose crew target is lowered below its crew: those over it
  wait inside for a ride to a barracks (a hub or accommodation block)
  with room for ten;
- a barracks sending crew to a building: an accommodation block always
  sends them by APC; the hub lets them walk when its own pool covers the
  job;
- a transporter pad, after each beam-down, to carry the people on.

It drives to where the people are, takes them on (ten a ride) or sets
them down at the door, and always drives home into the hub afterwards.
A human side's requests wait for an APC that's free; the pool never
orders one built. One made at a vehicle yard drives straight to a waiting
request, else into the hub. An AI side gets one built free for each
request no APC is free for. A hub destroyed takes the vehicles inside it
with it. The player has no control that brings one out.

In Reluva the player's transports work this way (`MissionSim`, with the
economy's rides); a ride's stops last 4 s each (the game's 80 ticks at a
building without a dock; its dock steps weren't timed), and an APC
lost on the way loses its passengers. The AI sides' APCs stay in their
hubs and their people walk.

**The stop.** An unarmed APC first drives to a point twice the
building's model radius out on its own side. At a level dock (hubs,
yards, processors, pads) it waits for the door, flies to ARRIVAL, checks
RELEASE is clear and leaves by DEPARTURE, each leg distance / 100 (its
played dock speed), at least 10 ticks; at a vertical dock (mines,
sickbays) it docks if the dock is free, else tries again after 80
ticks.

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

### Briefings

Each mission's registration points to a briefing table per side (the
Federation's, the Klingons', the Romulans'), a list of 60-byte steps
played in order. A step gives:

- its **minimum length** in script seconds: it lasts the longer of that
  and its clip;
- its **clip** (looked up in the side's voice folder; `nospeech` silent;
  the tutorials' in `MISC`);
- the **page**: a string of the mission's table, and where its box goes
  (no box: the page isn't shown), plus an inset box;
- the **view**: the relief map, the ground camera on a named object, that
  object's model turning on the grid, space (the galaxy, `~Tabula Rasa`,
  or a star system), or the side's emblem;
- one or two **named objects** (placements or groups; with two the camera
  works between them) and a camera mode;
- a **hold** flag: at its end the step tells the mission's briefing
  script (its phase 2) the scene may start, and waits for its `show`. Seven
  steps hold (F1006, F2006, F3006, J4R005, J4R009, R2006 and MarkL1's
  F1006): the script then flies a unit placed for the briefing (a shuttle,
  R2's Bird of Prey) along its waypoints at a set height (9000-12000), and
  the steps after follow it.

Every briefing opens with silent emblem steps (2 s, the incoming
transmission page 5 s, 2 s) and ends with a short silent one. The tables
settle what the recordings couldn't: some clips are never played (F14013,
K14009, K8006-K8008, J1F007-J1F010, J7F014), some twice (f14005, k14007,
J4R011), and R8's, never recorded, is all there. Reluva plays the tables
as they are (`briefing_data.cpp`, generated), the scenes' units flying
their paths from the first step that follows them, at the shuttles' 100 a
tick (their models by race: `shuttle`, `kcs`, `rs`, likely).

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
a few natives' types carry several weapons.

**Targets.** Each armed object looks round every 8 + (its id mod 4)
ticks, in a square its turret's range across each way, for live, built,
uncloaked objects it's hostile to. It takes the nearest in range (range
plus the target's size), in its turret's arc and in sight; failing that,
its turret follows the nearest in the square, ready. Whoever is shot at
turns to the nearest attacker it can hit. A turret fires only when it's
on its target that tick; it turns speeding up and then slowing down
onto it, at 128 (yaw) or 64 (pitch) 65536ths of a turn a tick each tick,
with no top speed: half a turn in about 32 ticks. Moving units fire.

**Aiming.** Every turret, standing or moving, leads its target: it aims
where the target's last tick of movement takes it in the time a shot
takes to cover the distance (3D, from the shooter's middle) at the
turret's speed, in one step. The speeds: a bolt is aimed at 2700 a tick
but flies 5400 (the game doubles its launch once), so it leads twice
too far; a beam pulse moves its beam's length a tick (the model's
`front_lock` to `muzzle_flash`); a shell 350. Artillery keeps the
shell's speed and solves the angle for the aimed spot, on the low arc,
or the high one when its line is blocked; it then scales the speed so
that along that way the shell comes down at the target's distance now.
Shells fall an exact parabola, 3 a tick² (so a standing target is hit);
the flat reach is 36 850.

**Hits.** A shot hits the first object it passes that isn't its own
side's (allies' and neutrals' included), not only its target; bees,
unarmed APCs and effects (class bits 0x1dea0) are never hit, nor
targeted. An object's size is its model's bounds radius (a vehicle's
times 0.9, types 117, 261 and 264 half; a building's times its scale),
grown to cover attached parts: a phaser tank about 467, an APC 523, a
construction yard 2512, a vehicle yard 5192, an accommodation block
5818. The test is a sphere round the object against the shot's path
that tick, stretched a quarter at each end. One object takes the whole
damage; nothing splashes, shells included. A shot touching the ground
is spent. A pulse or bolt is gone at its range's end; a shell flies
until it lands, at most 1000 ticks. Ranges count to the target's edge
(its radius).

**Lines of fire.** A pulse or bolt turret picks only targets it has a
clear line to (from its middle) and fires only while it has one (from
its muzzle, looked at every 8-11 ticks); blocked, it gives up the target
and looks again (one the player ordered it at, it drives after,
planning its way again at most every 20 ticks). A line
is blocked by the ground, and by any object other than shooter and
target whose radius reaches the line: everything not hostile to the
shooter (its own, allies', neutrals'), and, for the colonies, hostile
buildings (an enemy's units and turrets never block). Artillery fires
over anything, lobbing high when blocked. Something that drives into
the line after the look is hit.

### Scans and captures

**What the playbook offers, by posture.** The game keeps a relation byte
per pair of sides beside the postures (friendly bit 1, hostile bit 2,
neutral bit 4). On another side's object a scout or science vehicle gets
the three scans when the player is hostile or neutral to it (a plain
click is a science scan); other vehicles get the attacks only on what
the player is hostile to; an armed APC gets capture only on a hostile
side's building that can be captured. A friendly side's objects get none
of these. Props (class 0x800000: the wrecked USS Explorer's two halves,
types 134 and 135, and the Metar SAD, 120) are in no menu and take no
cells of the building grid.

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

**Range and replies.** A scanner drives for the same kind of point as an
attack, about a target's radius off, and scans as soon as it is within
its sensor range plus the target's radius (18000 + r for a science
vehicle). A scan nobody watches gets the routine reply 2 ticks after the
scanner comes in range, once each spell of scanning; only F1, F2, J6, K1,
K2, MarkL1, R1 and R2 have one (elsewhere nothing is said). The raids'
and watches' delays are raw ticks.

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

**Faces.** Each officer has a portrait index in the mission's data, into
the race's list of 40 small (32 x 32, `ptrt/crew/sm`) and large (64 x 64,
`ptrt/crew/lg`) pictures; the card and the training list show the small
one. A message numbered N shows the large picture N - 1 of the same
list. The list mixes the race's crew, the natives', the other races'
player faces and a few repeats.

**Life support.** Everyone on the planet uses 16 units a tick of a
life-support pool (655360); farms add 2048 × their efficiency; the hub
buys the rest back with power before any other building, (what's
missing) / 16.384 / its efficiency. An empty pool was not seen to do
anything.

**Moving officers.** A click on an officer's card picks them; a click on
one of the colony's buildings or vehicles with a free officer seat moves
them there. There is no drag; the range is 2000000 units, so none. A
double click on a card takes F1's camera to whatever holds them.

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

**Rolling out.** The tick after the bar fills plays the ready sound;
the vehicle then leaves through the yard's dock: a tick, three times the
LOCK-RELEASE leg and the RELEASE-DEPARTURE leg at its speed: about 9.5 s
for a tank from the Federation's yard (6.5 s a scout), 4 s from the
Klingons', 7.3 s from the Romulans'.

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
**Officer training** runs in a queue of its own the same way, with its
own 1000 points a tick (so it doesn't slow research). The mission marks
which officers may train (62 of its 346 officer records); each of those
below 1.0 gets one course, 150000 points, at the start. The player
starts it with a click on its button (the Operations panel's Training
tab and the STAFF window's TRAINING button, lit while a course is still
to be had), which reads the officer's name and train in / training in /
trained in their speciality. A finished course adds 0.25 to the skill
(1.0 at most), nothing else: no rank, no message, and it isn't offered
again. Officers keep working where they are while training. AI sides
don't train. Skills read by their quarter: UNTRAINED, BASIC, SKILLED,
HIGHLY SKILLED, EXPERT (1.0). Titles come by race and the record's rank
number: the Federation's Ens., Lt., Lt.Cdr., Cdr., Capt., Comm., Adm.
and Dr. (a doctor); the Klingons' and Romulans' their own. The science station's labs are add-ons bought with
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
- **The tasks.** An order's task code says what it does: 1 a patrol
  that loops (back to its first waypoint after its last, for good; with
  one waypoint it goes there and stays); 2 a sweep, one pass along its
  waypoints, ready again once it's done (outside modes 3, 5 and 6, only
  while a hostile, alive, built and uncloaked object stands within 50000
  across and up of its last waypoint); 3 and 9 builds; 4 a guard of its
  first waypoint; 5-8 an assault on side 0-3 (task less 5); 10 inert,
  never ready. Delays apply to tasks 1-3 and 9 (patrols and sweeps in
  modes 3, 5 and 6 go at once); guards and assaults have none, and an
  assault is ready only while its target side has anything left.
- **Legs.** Each unit sets off on the first leg as soon as the group has
  it; the later legs wait for the whole group. A patrol or sweep in mode
  5 or 6 ends as a guard at its last waypoint: mode 5 of its side's
  objects within 30000 across and up of the unit as it gets there, mode
  6 of itself. A guard in mode 4 protects every object of its side; in
  modes 2 and 3 it holds its spot, firing at what comes in range. An
  assault, past its waypoints, goes for the nearest object of its target
  side, each unit first chasing a live attacker of its own (no fog test),
  until the side has nothing left.
- **Messages.** Patrols and sweeps read theirs (`Abort` ends the group);
  guards and assaults don't read the text, so any message, `Abort`
  included, triggers them.
- **Done and lost.** A wiped-out group is followed by another, up to
  three in all, except in modes 3, 5 and 6; an assault instead waits for
  a new message.
- **Nothing else.** No building list, unit production or repairs of the
  AI's own were found; it builds only what its orders say, and rebuilds
  only through task 9. Every side has the same commander, ministries and
  buildings, so power, crew and officers set an AI yard's pace as they do
  the player's. Upgrades are always charged, but no order asks for one.

### The colony overview

With nothing selected (or a patch of ground clicked) the NEW WORLDS panel
shows the colony: COLONY SIZE n / limit, the six materials, then JUMP TO,
ten 32 × 32 icons in two columns of five, and the Build button. The left
column has a slot per class of the side's own objects, each with its
race's portrait: the hub; the mining stations (all with class 0x8); the
defensive weapons; the defensive artillery; the fighting, science and
scouting vehicles (class 0x4440100: not bees or transports, shown by the
armed APC's portrait). A click moves the camera (not the selection) to
the next such object after the one it went to last, round again after
the last, skipping those evacuating or destroyed. The right column is
battle groups 1-5: blank until the group is made (Ctrl + digit), then the
first member's portrait; a click selects the group and moves the camera
to it. (The red icon seen in recordings is most likely the one under the
pointer.)

The hub's **Info** button switches its panel between the hub's own pages
and the colony's details: the people (the roster's working, pool, dead,
average health), then a page for each building ordered and not finished
("building i of N") and each upgrade ("engineering i of N").

**The sickbay's block** counts the side's plain crew (officers not
included): assigned = the roster less officers and those in orbit (the
dead stay in it), infected = those in the disease slots; healthy =
assigned - infected, sick = infected - dead, dead = the roster's dead,
and the average health of the assigned (health out of 256; the dead
count nothing). The sickbay heals no one: disease takes no notice of it,
and its wards only raise the colony's limit. **A farm's block** is the
colony's life support as a producer: consumption (drawn over made last
tick), demand met, and the reserve (stored over 655 360): every farm
shows the same.

### The Tricorder

The small Tricorder (bottom right) has three buttons. **ZOOM** steps its
scale: X2 at the start, then X1, then X4, round again; the small map is
32, 64 or 16 tiles across round the camera (the full-screen one, Tab,
twice as many). **COMMS** opens the communications panel (a log of the
messages: one at a time with the sender's portrait and i/n, Delete,
Done, Mission; again it doesn't close it). Its X only hides the panel,
keeping the log; any new message for the player, COMMS or a chat line
brings it back (sliding in from the top in F1, from the bottom in F2
and F3). No key opens it. **ALLY** lists the other races
in the mission (from its Defence database), each a click to pick
Friendly, Neutral or Hostile: it sets only the player's posture towards
that race (through the order queue, as a script's posture switch does),
theirs unchanged; set Neutral, it turns Hostile at the first shot that
race fires at the player (a provocation limit of 1).

**Colours.** Blips are each side's colour, whatever the player's posture
to it: side 0 (the Federation) blue, 1 (Klingon) orange, 2 (Romulan)
green, 3 (Taubat) and 5 (Metar) yellow, 4 (Hubrin) magenta, 6 (Orion)
red. Alerted objects are red. Blips are outlined in black, the outline
blinking white every 10 redraws on the selected and alerted. The ground
and ores come from the map file's own colour table (its last 1024 bytes,
the terrain codes' colours): the Tricorder copies those 19 colours into
twelve bands of 20, each band shifted in hue, saturation and brightness
for its class and the player's race (the ores alike for all races:
dilithium +270 degrees, talgonite +30, silicon +210, kelbonite +330,
magnesite +90, dolamide +150, brightened 1.33-1.5 times), entry 0 of each
band the race's background (a dark blue, brown or green). A pixel is the
terrain code there in its block's class band (surveyed ground,
unsurveyed, an ore), so code 0 shows the background. Blocks within 5 of
the map's edge are drawn in the dark border band; class changes between
blocks are blended through 16 edge shapes, and unsurveyed blocks are
hatched (a ring, a dot or grid lines by the map scheme).

Its line names what's under the pointer: an object, by the player's
posture to its side (Friendly, Neutral, Hostile) and its class
(defenses, structure, unit), "Debris" for one gone; Impassable ground;
an ore (`<ore> at <a:b>`) or the terrain (`Terrain at <a:b>`); with the
pointer elsewhere, the view's name and the zoom (Relief X2).

### Cameras

F1, F2 and F3 set the camera mode, Q steps through them: F1 low over the
ground (2000 up, looking level; it can tilt, and numpad 5 turns it
round), F2 isometric (28000 up, 45 degrees down), F3 close-up isometric
(15000 up, 45 degrees down). The arrows speed up by 80 a step to 6400;
in F2 and F3 the pointer on the screen's outermost pixels scrolls the
same way. Z keeps the camera's position (one place; not its direction or
mode), X goes back to it. Space goes to the next of the side's objects
with an alert (hit, or a report: everyone beamed down, a mine exhausted),
in their list order; an alert lasts 41 Tricorder redraws.

A double click on one of the side's own objects in F1 puts a camera on
it: behind it at 5 times its model's radius (times distance / 800), 0.8
radii up, turning as it turns, and looks at the object's origin with the
up kept vertical: 9.1 degrees down at any distance (the offset is capped
at 12288 before the scaling) until 7 / 9 / 1 / 3 move it. The view's
width doesn't change. The distance starts at 1000; Up and Down
bring it in and out between 800 and 2000. Numpad 4 / 6 orbit the object,
7 / 9 raise and 1 / 3 lower the camera; the other arrows and edge
scrolling do nothing. F1, F2, F3 or Q, or the object's loss, end it.
JUMP TO and Space attach the same way in F1. In F2 and F3 a double click
leaves the camera where it is and only plays the object's own sound.
Every mission starts in F2.

**Limits.** The camera's point is held within 440000 of the map's
middle each way (about five blocks in from a 64-block map's edges) and
nowhere else: a check that would keep it over blocks the side has seen
exists but is switched off. F1, F2 and F3 reset it to 2000, 28000 and
15000 up (four other presets are never reached); only F1's height moves
(numpad 7 / 1, up to 128 a step), and every mode keeps it at least 800
above the ground under it and 800 ahead. Numpad 8 / 2 zoom between 0.75
and 8 times (reset to 1 by the F keys), turning towards the point under
the pointer as they zoom in.

### Keys

The game's own: X (camera back; Ctrl+X quits), Escape (quit, with a
yes / no), F1-F3, 1-5 (a battle group; Shift goes to it, Ctrl makes it),
P, A, T, S, G (all the side's artillery, APCs, tanks, science vehicles,
ground scouts), Space, Home (the hub), B (the build window; Shift or
Ctrl the next building), V (the vehicle yard's; Shift or Ctrl the next
fighting vehicle), C (the colony's details), Z, Q, Pause, Tab; in
multiplayer Enter and Ctrl+K, R, F for messages. The arrows and numpad
are read directly.

Escape opens one box asking whether to quit, yes or no; the mission
carries on while it's up, and yes goes back to the menus. Pause (and the
window losing the focus) freezes the game: no simulation, no game time
(the script's timers included), no input on the map; nothing on the
screen says so.

### Sounds

Each of a side's vehicles carries three groups of its type's voice
recordings: picked at its making, one of the type's two voices (the a /
b-style letter) for life; each time a group plays, one of its two lines
at random. Only the first object of the selection speaks, and only the
player's own:

| Group | Played when |
|---|---|
| 1 (`fapc003` / `004`) | a new selection is made |
| 2 (`fapc007` / `012`) | an order is taken (a move's once the first unit has planned its way) |
| 3 (`fneg1`) | no order fits what was clicked, or the move finds no way |

The sets: Federation `fapc` (armed APC 56), `ffgs` (97), `fmsg` (154),
`fpa` (9, 47), `fpv` (8), `fsv` (62); Klingon `kapc` (40), `kdt` (10,
229), `kmdb` (228), `kpa` (36), `kfgs` (37), `ksv` (95); Romulan `rapc`
(64, 230), `rct` (35, 148), `rfgs` (98), `rpa` (87), `rsv` (63). The
unarmed APCs (55, 88, 89) have none. Each group is four recordings: its
two lines in the first voice, then in the second (`ffgs`: `d` then `c`;
`fmsg`, `kmdb`: `e`, `b`; `fpv`, `kdt`, `rct`: `d`, `b`; `fsv`, `ksv`,
`rsv`: `e`, `c`; the rest in letter order); a unit keeps both lines of
its one voice.

Every vehicle (any side's, natives too) plays its start-up sound as it
sets off on a path: each move order, each re-plan of an attack's
approach, a scan's or capture's approach, the drive to the rally point,
a protecting unit's way back to its spot. Once a move, from where it set
out (it doesn't follow the vehicle), volume (65536 − distance) × 127 /
65536, so silent from 4 blocks; not over its own still playing; every
unit of a group its own. The clips: `fpvstart` (8), `fapcstar` (55, 56),
`fmsgstar` (154), `fpastart` (9, 47), `fscostar` (97, and the Klingon
scout 37), `fsvstart` (62), `ftugstar` (199) in `fedactor.cat`;
`kapcstar` (40, 88), `kdtstart` (10, 229, and the Klingon artillery 36),
`kmbstar` (228), `ksvstart` (95) in `kliactor.cat`; `rapcstar` (64, 89,
230), `rctstart` (35, 148, 149, 200), `rpastart` (87), `rscostar` (98),
`rsvstart` (63) in `romactor.cat`; the natives' `tgpstart`, `hstart`
and `mras` in `foractor.cat`. Bees have none, and nothing loops while a
vehicle drives. (The game also turns every new sound down when those
playing already add up past full volume; Reluva's mixer sums them.)

The officers report by number, in the player's race's one recording each
(Media/Misc): 001 a vehicle delayed (the yard short of crew or
materials), 002 a building delayed for want of materials, 003 an
upgrade delayed, 004 a colony hub finished, 005 a vehicle finished, 006
a yard's power below 0.2 (production suspended), 008 back above 0.35
(007 never plays). Each report at most once in 600 ms; none plays over
one already playing. A finished building of the player's hit plays the
colony's danger music (Media/Music: `fedcoldanger`, `klinColonydan`,
`RomColonyInDangr`) and its "under attack" voice, at most once in 30
seconds. Other sides' reports are never heard.

A double click on a finished building plays that building's own sound,
from its race's build archive (`fedbuild.cat`, `klibuild.cat`,
`rombuild.cat`): the hub `fcomstn` / `kchub` / `rchub`, the vehicle yard
`fvyard` / `kvyard` / `rvyard`, and so on, one per building (upgrades and
advanced versions share their base's; the Romulan shield generator has
`rombuild.cat`'s own copy of `fshield`). Vehicles and the natives have
none. Only the player's race's are loaded, so only the player's buildings
sound. It plays from the building: volume (65536 − distance / 2) × 127 /
65536 of the game's 127, so full at the listener and silent from 8 blocks
off; panned by how far to the side it lies; one at a time (a new one
stops the last). Reluva listens from the camera's spot on the ground.
Vehicles' start-up clips are their drives', played as their moves start
(above).

The executable's sound table pairs each resource id (archive number <<
24 | the entry's index in it) with a side filter and the slot it fills.


### The multiselection

More than one object selected shows MULTISELECTION in the NEW WORLDS
panel: pages of 15, three across, each a 46 × 48 cell with the object's
32 × 32 portrait and a 32 × 3 health bar under it (green for the share of
hit points left, red the rest). A right click ticks or unticks an entry
(a frame round it, colour 0x0080ff); a click selects that object alone;
a double click is a double click on the object. DELETE drops every
ticked object from the selection; INVERT flips the ticks; the arrows
turn the pages.

### Stores

Only storage facilities put their store on the side's list, and the
colony's stock is that list's total: the hub has no store, and what a
processor (its ore and refined goods) or a mine holds isn't counted, and
goes with them. What comes in - the mission's starting stock (once the
starting colony stands), refunds, the script's gifts, the processor's
goods every 32 ticks - goes to the first facility with room (in practice
the first: each holds 2000000000 units); with none, it's lost. A
building's cost is checked against the total and taken from the
facilities in the order they were built, each material until it's paid.
A vehicle's cost (and the construction yard's own spending) comes only
from the side's first storage facility ever, checked and taken when
production starts and put back if the order is deleted; so a yard can be
short while the colony's total isn't, and if that facility is destroyed
the yards keep using its store, which no longer counts in the stock. A
storage facility destroyed takes what it held. The computer's sides pay
nothing (a per-side flag set only for human players).

### Demolishing

The panel's raze button asks yes or no, and works only on the side's own
finished building that isn't a hub or storage facility and isn't already
evacuating. Yes gives back half the type's cost at once (each material
halved, rounded down; upgrades bought aren't refunded) and starts the
evacuation: the building's crew target goes to 0 and its people walk to a
hub or accommodation block with room; it waits for its docking bays and
vehicles to empty and its jobs to finish. Then it takes its own hit
points in damage and dies the usual way (the destruction effect; no
under-attack alert), so it counts as destroyed. A site can't be razed or
cancelled by the player; an aborted computer or script build order tears
its site down with the full cost back.

**How long.** Razing sets the building's crew to 0: the crew wait
inside and it asks for an unarmed APC, which takes them aboard; the
building goes once the APC has left its dock (on a human side the APC
must exist: without one it presumably waits). Two blocks from the
Federation hub it's gone about 30-35 s after DEMOLISH, its crew in the
hub's pool after 55-65 s, about 19 s more a block each way.

### Bees

- **Where they are.** A side's bees belong to its hub (the vehicle pool's
  only garages are hubs). A bee docked there is off the map: not drawn,
  not hit. It's out only with a job, or on its way home. On a human side
  the vehicle yard builds worker bees when asked for one; construction
  and cargo bees are never made on demand, the jobs wait for one that's
  free. Bees can't be hit (their hit points are set out of reach at
  start-up).
- **Flying.** Bees fly straight (no paths), a timed flight speeding up and
  slowing down, of distance / speed ticks and at least 10: 150 units a
  tick for construction and cargo bees, 200 for worker bees; construction
  and cargo bees cruise in eight lanes 9000 to 12500 up (each the
  least-used), worker bees 500 up between their building and its yard.
  (The paces in the table are for driving units.) In and out of a
  building they go through its dock (below).
- **Building.** A site asks for four construction bees; each takes a free
  spot at its corners (2000 units out each way). The building is put
  together a piece at a time: its model's four corner sections, each cut
  into tiles of about 800 by 1200 units, one arm movement (16-17 frames)
  and a short flight (10 ticks or more) a tile. So there is no time per
  type: the work is the model's tiles, shared by the bees there, and one
  bee takes four times as long as four. A bee done with its section moves
  to one no bee is on; with none left it goes home. Finished, the site
  lets all four go, home or on to a site waiting for bees. A bee lost on
  the way is replaced; the work done stays. No power or crew enters into
  it.
- **Carrying.** Goods go in cargo pods (types 19, 60 and 79), 80 t the
  Federation's and 160 t the Klingons' and Romulans'; drawn as the
  Federation's crate (`crate_l`), the Klingons' `kwbc` and the Romulans'
  `rc`, each with its `_s` shadow. A mine that holds a pod's worth packs
  its stock into pods, evenly; it stops while four wait. Every mine,
  processor and storage facility has a worker bee of its own, which only
  moves pods between its building and its **yard**: 16 places in an L
  along the two outer edges of the quarter block its dock's RELEASE
  point is in (256 in from the edges, 992 apart, the corner the ninth),
  filled from the far end; aligned to the map, whatever way the building
  faces. Docked inside, the worker bee takes a pod out when the yard has
  two places free more than pods are coming: out of the dock, to the
  place, a tick and a 30-tick lift to set it down, back in. Ore goes to
  the first processor with room (the list's order, not the nearest),
  refined goods to a storage facility; a processor packs its refined
  stock for storage every 32 ticks. A building with more than one pod
  coming asks for a cargo bee, which carries two a trip: it waits
  (checking every 16 ticks) until its building's yard has two places
  free and two pods are ready (after 64 checks, about 51 s, it goes for
  one), flies to them, takes each up (a tick, a 50-tick climb), flies to
  its own building's yard and sets each down (a tick, a 30-tick lift, a
  50-tick climb). It never docks there: the building's worker bee takes
  each pod in (out of its dock, to the pod, a tick, back in; the pod's
  contents go into the store and the pod is gone). Lost bees are asked
  for again. While the starting colony is set up, pods arrive at once.
- Not found: the number of tiles each building's model gives (so the
  build times; the training missions' timings stand in, with four bees).

**Speeds as played.** The start-up rewrite sets the bees' flight speeds
to 1000 units a tick (construction), 600 (cargo) and 400 (worker), and
the unarmed APCs' dock speed to 100 (the stored figures, 150, 150, 200
and 500, are never used). Most bee dock legs so take the 10-tick
shortest; four bees leave the Federation hub about 2.2 s apart.

**How they build.** The bees build a building's deconstructed `_d` model:
its file ends in a table of four sections, each a list of faces, between
them every face. Each face is cut into tiles of up to 800 by 1200 (empty
ones skipped); a bee builds one tile a cycle (an arm swing of 17 frames
and a reposition flight, about 18 ticks by the code), in its own section
only, then takes the first unfinished section without a bee. So four
bees take as long as the biggest section, one the sum of all four. The
Federation's timed builds fit T = max over sections k of (5.5 s + 2.2 s
x k + 0.67 s x its tiles) within about 4.5 s, which gives the untimed
ones: shield generator 27 s, photon turret 22, artillery 31; the
Klingons' generator 21, yard 40, shield generator 22, turret 24; the
Romulans' generator 25, yard 62, shield generator 48, turrets 29-33.
(The per-tile time the fit needs is less than the code's cycle reads; the
relative sizes are firm.) An AI builds defences and shield generators
the same way; one without an idle construction bee gets one made free.

**Cargo.** A cargo bee never docks at the mine or the processor: it
takes and sets down pods on their pads, about 8-9.5 s for its two at the
mine and 11 s at the processor; inside, a worker bee spends about 8.5 s a
pod at a Federation mine and 6 s at the processor. A trip carries two
pods: 80 T for the Federation, 160 T for the others.

### Docking

Every building has a dock: its model's points VEHICLE_DOCK_ARRIVAL,
_CAPTURE, _LOCK (where a vehicle parks inside), _RELEASE and
_DEPARTURE. Going in: a tick, the flight from ARRIVAL to CAPTURE, the
lock leg to LOCK, 5 ticks; coming out: a tick, the release leg to
RELEASE, the flight to DEPARTURE (an unarmed APC stops at RELEASE). Each
leg takes its length over the vehicle's speed (construction and cargo
bees 150, worker bees 200, unarmed APCs 500), at least 10 ticks; the
lock and release legs three times that (and play the building's door).
At a level dock (CAPTURE to LOCK not within 10 degrees of straight up:
hubs, yards, processors, pads) the heights on those legs don't count; a
mine's and a sickbay's are vertical. So a bee takes about 111 ticks into
the Federation hub and 122 out (ARRIVAL 6514 out along the model's −y
and 5300 up; LOCK at its middle), a worker bee 46 into a mine, an APC 47
into the hub and 42 out. Only one vehicle uses a dock at a time, coming
or going; others wait where they are (a bee hovers), those leaving
first. So four construction bees leave the hub about six seconds apart.
Reluva reads the points from each building's model (`MissionSim::
useModels`) and keeps each dock to one vehicle at a time; without the
models (the tests) a dock takes 20 ticks each way.


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
- **How it looks.** Each unit keeps a fade step (0-63) and, for its
  own side and the others, a pair of 64-entry tables. While it fades,
  every vertex of its model is thrown a random distance each frame, up
  to about 100 world units: for the player's own most in the middle of
  the fade (about 97), for another side's growing to about 98 as it nears
  cloaked. Fully cloaked or fully seen, it doesn't shimmer. An own unit
  also gets an opacity of one half plus step / 128, but nothing reads
  it: cloaked, it is drawn as it is.
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
| Protect from location (limited range) | location kind 4 | drives to the spot and holds, firing within weapon range; every 8 ticks, if anything of the side has a live attacker, chases the nearest, with no leash, until it dies, cloaks or stops being hostile, then the next, and returns only when none is left |
| Protect area (mobile shield generator) | target kind 5 | guards the clicked buildings from where it stands; when one is attacked it drives to the attacker, its field with it, and returns; it never fires |
| Return to vehicle rally point | no order | the unit is handed to the side's field marshal |

**Live attackers.** Each object remembers one attacker: the shooter of
the last shot at it, replacing the one it has only if nearer. It stays
"live" until it dies, cloaks, stops being hostile or is being captured;
no time, distance or fog is tested, so a unit that shot once and drove
off still counts. Follow and attack ends only when its target fails the
same test; the 20 ticks without a shot just make it look for a new
firing position.

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

**Out of the arc, and groups.** A unit told to attack a target its
turret can't bear on doesn't pivot: it plans an attack point (a sweep of
37 points 10 degrees apart, growing 1.25 times a round), about a target's
radius off on its own side, drives at it and fires once the target is in
the arc. Only the Klingon armed APC (40, 90 degrees each way) and the
Romulan APCs (64, 230, 60 degrees) have limited arcs. Hulls turn with the
turrets' speed-up-and-brake rule (192 units a tick squared for tanks,
APCs, artillery and science vehicles, 16 for scouts and bees, 32 for large
craft; a half turn about 26 ticks). Units sent together all get the same
point, starting 0-11 ticks apart; they pack round it by their boxes,
about 1024 apart, and one whose next step is taken waits 5 updates, then
plans again.

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
sends Go_Futile to the raids it names (targets 4 and 5) when its message
is Go_Futile: in J2 the three hangar captures name each other (not the
hangar's destruction monitor), in J3 each workshop's three raids; J4's
and J6's raids on the same buildings send other messages and hand
nothing off. A raid told so while under way reports "beaten to it"
(target 1); one idle or still in its start delay turns futile and later
reports "futile" (target 6) instead of success.

### The selection panel

One object selected (a building's details need exactly one) shows in
the NEW WORLDS panel, its class picked by type id:

- **Every object:** its name; its side (table: Federation, Klingon,
  Romulan, Taubat, Hubrin, Metar, Orion, Rival, Rebel), the holder's when
  captured; a state word, later rules winning: OPERATIONAL once built,
  else INCOMPLETE; CAPTURED when held; EVACUATING; DESTROYED below 1 hit
  point. A vehicle shows EN ROUTE while moving, else READY. SHIELD COVER
  is High when the object's block has its own side's shield-field mark,
  else Basic (None if destroyed). A bar shows its hit points' share.
  Another side's objects show only this.
- **The colony's buildings** add EFFICIENCY (× 100, cut down: 1.2 shows
  120 %). Consumers show CONSUMER, SUPPLY (the running average of the
  share of power granted; 100 % when nothing was asked) and RESERVE
  (their buffer's (4 × average + now) / (4 × capacity)). The hub and
  generators show the colony's pool instead: GENERATOR Low (hub), Med.
  (generator), High (advanced generator), by type; CONSUMPTION (power
  drawn over made, last tick); DEMAND MET; RESERVE (stored / 6400 %).
- **Extras by building:** OFFICERS / CREW / POOL from its people (hub,
  security centre, science block, pad, accommodation); the hub and the
  construction yard the side's WORKBEE, CARGOBEE, CNSTRCTNBEE and
  TRANSPORT counts (the yard also colony size and the stock); the
  vehicle yard its materials and MANUFACTURING with a bar, or SUSPENDED;
  mines and the processor their raw ores; storage its materials; a
  shield generator its COVERAGE High / Med. / Low (its field's size);
  farms a producer block from life support; the sickbay its healthy,
  sick and dead; upgrades on offer.
- **Buttons:** BUILD (the build menu, or the vehicles on a yard; only
  under the colony's limit), DEMOLISH (with a yes / no; not the hub,
  storage, an unfinished or evacuating building); the hub a roster window
  and a toggle to the colony overview (size, stock, JUMP TO); the sickbay
  and science block a window and an assign button.
- **The build and vehicle menus** have three cells a page, each 90 x 60;
  a picture is drawn at its own size, cut to the cell, so the buildings'
  64-pixel portraits show 64 x 60. They come from the large per-type
  list, which only these menus use; JUMP TO, the multiselection and the
  queues use the small 32 x 32 one.
- **Loading boxes** have no set time: they stay while the loading lasts.
  The briefing and debriefing wait for their button, with no time-out.
  The mission report window has a fixed "Mission Log" heading (no
  accomplished or failed line: every mission ends with the same two
  scenes, and the missions' own pass and fail strings are never shown),
  four rows a page: won, each objective, complete or incomplete; lost,
  only the fail criteria that fired, failed. Behind it, the race's
  emblem on a win (the `f_win` tiles are never used), the lose tiles on
  a loss.
- **COMMAND** (the objective list): four rows a page, an icon and each
  objective, incomplete or complete; hidden objectives join when a
  message first names them; no fail criteria. It rebuilds every 10
  ticks while open; its button only opens it.

## Still open

- The seconds a bee takes a tile (the code's cycle against the timed
  builds).
- Placement flags and a third placement phase.
