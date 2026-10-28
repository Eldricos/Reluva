# Star Trek: New Worlds: a Federation Let's Play (episodes 016-029)

Fourteen episodes of a YouTube Let's Play of the Federation campaign
(1280×720 at 60 fps, the game's 4:3 picture filling the width with black
bars above and below; commentary over it), about 9½ hours. Observed
behaviour, as in the other play pages; times are each episode's (mm:ss).
The messages' wording isn't copied. October 2026. The reviewers' full
notes (every message, with times) stayed in the working folder; this page
keeps what answers or changes something.

| Episode | YouTube | Mission | Notes |
|---|---|---|---|
| 016 | `t8Qs3c8Rmfo` | F1 Power Corrupts | restarted at 31:29, won at 45:20 |
| 017 | `qs06vuZwxDo` | F2 A Harsh Lesson | ends before the mission does (at message 19, the cure) |
| 018 | `WfxxzFKmr1A` | F3 A Piece of Paper | primary objectives complete 17:19 |
| 019 | `-2c3wvbRplU` | J1F Into the Arena | won; the player's own force destroys the Klingon hub (41:08) |
| 020 | `RQyus7N544I` | J2F Absent Friends | won |
| 021 | `roW8-S9RHps` | J3F The Mask Slips | won |
| 022 | `obigbKE5tU0` | J4F Cry Havoc | won (9 messages) |
| 023 | `mteLRYdiiEg` | F8 All that Glitters | lost at 23:26, replayed and won at 41:56 |
| 024 | `eLxuO9D_d5E` | J6F Keep Your Friends Close | won (24 messages) |
| 025 | `0N-cWfnTLjI` | J7F The Jaws of Death, then F12 Death Rattle | both won |
| 026 | `Y4RGA822FEQ` | J8F Checkmate | lost at 22:50 (vehicle yard destroyed), replayed and won at 35:24 |
| 027 | `up1q_ETHHW0` | F14 The Finest Hour | won |
| 028 | `A_KJ1G9165Q` | J9F The Lost Tribes | won, with the Federation's movie |
| 029 | `WN2DHzCv6sE` | C9 Slaver's Run | won |

The player deletes messages often, so the COMMUNICATIONS counter there
counts the messages held, not the mission's message number.

## The Tricorder and ore names

- **Only the full-screen Tricorder (Tab) names an ore** (and the briefing's
  relief-map step, F3 2:47). Its readout is printed at the top centre of
  the screen, where the compass strip goes, while the compass moves to the
  small Tricorder panel's line; the small panel shows the camera's view.
  The readout reads the map under the fixed `+` at the map's centre (the
  camera's target), not the mouse pointer (J1F 8:12: dilithium named with
  the pointer far away).
- **The main view never names one**: the strip over it is always the
  compass, and the small Tricorder panel's line reads RELIEF X2, TERRAIN
  AT or IMPASSABLE (also over the small relief inset, F14 4:06-4:07,
  TERRAIN over dilithium blocks).
- **The readings fit the maps**: across F1, F2, F3, J1, J2, J3, J7, J8,
  F12/F14 and C9 several hundred ore names were checked against the maps'
  deposit kinds, and all but one lie on a block of the kind named (the one
  beside its patch). That confirms the ore table in
  [gmf.md](gmf.md#resource-deposits) for every kind; kind 5 reads
  "MAGNESITE ORE AT", two words, which is why its column is cut off.
- **Only revealed deposits are named**: a block the Tricorder doesn't yet
  draw hatched reads TERRAIN (F14: TERRAIN on a dilithium block at 4:07,
  DILITHIUM on the same block at 7:56; J7: one block TERRAIN on the
  briefing map, DILITHIUM later in play once scouted), as Reluva has it
  (the block surveyed). A few TERRAIN readings on deposit blocks were taken
  while the map scrolled, so the rule may be finer than whole blocks.
- The hatching's colours: green dilithium, red dolamide, magenta magnesite
  (C9); likely cyan kelbonite and yellow silicon.

Reluva now draws the Tab Tricorder's readout this way (at the top, for the
camera's target, marked with a cross; the compass on the small panel's
line). The small panel doesn't yet show the camera's view, and the map
isn't the game's tilted relief.

## J2: the mine at 196, 436

Re-read enlarged in J2F (020, 6:44): it does read "196, 436", as in the
other J2F play, three minutes into play (message 4 in both). It is one of
the starting colony's own mines: the mission places three advanced mining
stations, at <176:407>, <192:439> and <216:415> (`ai_data.cpp`), the
second in block row 24, column 54, reported as 196, 436 (the block's
middle). That block is kind 0; the nearest silicon patch is 2-3 blocks
off, within a mine's 7 × 7 reach ([exe.md](exe.md#bees)), so it digs that
until it runs out, soon. The third is the 220, 412 the other J2F play saw
run dry. Reluva places and digs them the same way. A later mine of the
player's (156, 364 at 25:49) lies on kind 3.

## Star-system views

In every briefing the camera holds still on a small body at the screen's
centre while the others orbit it, every 10-12 s: J1 a cyan-teal banded
one, J2 grey-green, J3 dull red, J7 small tan, J8 a ringed green-teal one,
F8 dark red, F12 and F14 a tiny green one, J9 a dim teal ringed one. The
planetary-data step names the mission's planet, and the camera centres on
it, so **the centre body is the planet of operations**. Nothing glows and
every body is drawn half lit, so which (if any) is the star isn't shown;
if the star is drawn as a body it would be the large banded one that
sweeps across the frame (yellow in J1, yellow-green in J2, orange-cream in
J3). J4's view shows only the yellow-brown haze; J6's is almost black.

## Units and buildings named on the selection panel

- **J7's grey-blue crystal units** are Romulan DISRUPTOR TANKs (025, 16:42).
- **The red-and-white fliers** (J7, J8, F14) are the Metar's: Dea, Sniper,
  Bur, Ras, Nak, Advanced Yat, Advanced Dea, Advanced Nak, Advanced
  Sniper, Starship.
- Metar buildings: Active Portal, Metar House, Metar Temple, Metar Colony
  Core, phaser defense turret, mobile shield generator, containment pillar
  (F14), and buildings they captured ("captured accommodation", "captured
  science block").
- Taubat: tank, artillery, phaser defense turret, house, temple, hangar.
- Hubrin: a unit named HUBRIN (the observer, F12 38:12), HUBRIN FORTRESS (J2,
  F3, F8: a large dark structure with two boxy arms on a mesa).
- Orion disruptor tanks (C9). A Romulan photon defence (F3).
- J7's magenta square on the briefing map (at the colony's north-west
  edge) is drawn like a unit's marker, not a deposit's hatching; what it
  marks isn't seen.

## Starting colonies

| Mission | Colony | Dilithium | Polycomp | Thermoconc | Transalumin | Nitrium | Duranium |
|---|---|---|---|---|---|---|---|
| F1 | 20/50 | 500 | 1000 | 2300 | 500 | 4000 | 5000 |
| J1F, J3F | 26/50 | 500 | 1000 | 2300 | 500 | 4000 | 5000 |
| J2F | 35/50 | 500 | 1000 | 2300 | 500 | 4000 | 5000 |
| J7F | 39/50 | 150 | 1000 | 2300 | 200 | 4000 | 10000 |
| J8F | 41/60 | 660 | 1160 | 2300 | 660 | 5600 | 6600 |
| F12 | 47/90 | 960 | 1910 | 2300 | 1160 | 25600 | 26600 |

F1, J1-J3 and J7 start with the data's stock exactly. **J8 and F12 start
with 160 more of dilithium, polycomp and transalumin and 1600 more of
nitrium and duranium than the data's** (J8: the default 500 / 1000 / 2300
/ 500 / 4000 / 5000; F12: 800 / 1750 / 2300 / 1000 / 24000 / 25000), and
thermoconc unchanged: the same 160 000 game units of every material but
thermoconc. J8R's 656 / 1140 / 2211 / 653 / 5100 / 6600 at 2:46
([play-j8r.md](play-j8r.md)) is the same, partly spent. It is the vehicle
yard placed with those missions' starting colonies: made during the
set-up, it tops the empty store up to 160 000 units of each of those
materials before the mission's stock goes in ([exe.md](exe.md#stores));
Reluva does the same now. (K7 starts the same way, J4K at the data's
stock: J4's Klingon colony has no yard.)

## Messages worth noting

- **F1** (016): the restarted play's first ten, cleanly numbered: 1 the
  Klingon colony to the south (Intelligence), 2 construction yard
  finished, 3 a Klingon vehicle coming, 4 disruptor fire (Security), 5 the
  plague's epicentre (Medical), 6 the Taubat's warning to leave their
  sacred ground, 7 a scan running (Medical, arriving within a second of 6),
  8 not seen, 9 research at the sickbay (Medical), 10 vehicle yard
  finished. The Taubat warning and Operations' power warning both repeat.
- **F8** (023): message 7, missing from [play-f8.md](play-f8.md), is
  Command's "transporter locking on" (41:20), on screen for about 1.5 s
  before 8 replaces it.
- **J6F** (024): the counter reaches 24; 10-21 arrive during a cut in the
  recording (42:16-42:26); 22 Engineering's device in the Romulan hub,
  23 shields collapsing, 24 the Federation's mission complete.
- **J7F** (025): the counter jumps from 2 to 4 at 22:43 (two together, 3
  never read).
- **J8F** (026): a win also brings Command's "anomaly device destroyed,
  mission failed" 4 s after "mission complete"; the report still lists
  every objective COMPLETE.
- **J9F** (028): a "scan lost, try again" line, not in the J9 pages; the
  counter jumps 2 to 4 at 23:16 (the Science "study of anomaly device"
  never shown, as in J9R).
- **C9** (029): its first message, missing from [play-c9.md](play-c9.md), is
  a Romulan transmission (Imperator Travore) offering an alliance against
  the Orions; the whole four-message opening then arrives a second time
  about 2¼ minutes later, the panel opening by itself.
- **Repeats**: Operations' power warning about once a minute (J4F 13:59,
  14:55, about 15:52, 16:51; F14), as Reluva has it. The resource-shortage
  line re-arrives about every 5 s while resources stay short (J4F
  27:16-38:21, J6F 18:49-25:21): the vehicle yard's report, repeated as
  the colony's buildings keep asking for worker bees it can't pay for
  ([exe.md](exe.md#stores)). Reluva says it only when an order is
  refused.
- **J4F**: the Romulan hub is reported destroyed at 26:33 while the camera
  is on the player's colony (not the player's doing); buildings captured
  change hands back (the Taubat hangar Federation at 39:09, Taubat again at
  39:27).

## Maps

The ore readings and exhausted mines put each mission on its own map
(`f1`, `f2`, `f3`, `j1`, `j2`, `j3`; F14 on `f12`). **C9 plays on `demo1`**,
not `f3` or `j3`: all seven dolamide readings lie on `demo1`'s kind 6,
every magnesite one in its kind-5 rows, and its mine at 260, 300 on kind 5
(as level.md has it).

## Not seen

A Klingon AI colony's mining station, an upgraded Romulan hub close up,
build timings, who destroys J2's Klingon hub (no message for it in this
play), who kills F12's Hubrin observer.
