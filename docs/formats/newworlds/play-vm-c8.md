# Star Trek: New Worlds: the training missions recorded in a VM - C8 Combat

From the project's own recording of the game played in a virtual machine,
"Tutorials - Others", 55:15-100:19. The game picture is the 640 x 480 game scaled to 712 x 534; the
sound is poor, so some lines are read from the screen. Times are mm:ss in
that video file. Messages and pages are paraphrased. Summary and how this
adds to the earlier review of another player's recording:
[play-vm-tutorials.md](play-vm-tutorials.md) and
[play-tutorials.md](play-tutorials.md).

The stretch starts at the third Federation emblem screen (55:31-55:40)
and holds one continuous C8 game, 56:33-99:50; the video ends with C8's
report, the main menu and the Single Player Campaign screen.

## 1. Timeline

| Time | What |
|---|---|
| 54:56-55:23 | end of C7 (Defense): Engineering mine-exhausted message, then Command "tutorial 4 complete, loading next" (55:16-55:23) |
| 55:31-55:40 | Federation emblem with a CONTINUE button; a **MISSION REPORT** box (C7's) comes up at ~55:35: build a security center - COMPLETE; provide the colony with 2 (artillery?) - COMPLETE (half hidden behind the emblem in the frames) |
| 55:40-55:41 | loading box over the three emblems: briefing being set up, the name **COMBAT**, officer command routines loading, percent done (62 % in 55:41) |
| 55:42-56:33 | C8 briefing, 4 INSTRUCTION pages (see 1a) |
| 56:33 | play starts |
| 56:33-99:50 | **C8** played; completed |
| 99:40 | Command: Klingon hub destroyed (14/14) |
| 99:50 | Command: tutorial 5 done, all colonial-infrastructure tutorials finished, fit for a first training mission (15/15) |
| 99:53-100:01 | emblem, then the C8 **MISSION REPORT** (MISSION LOG; objective 1 build a viable colony with a stage 3 hub, stage 3 construction yard, stage 2 vehicle yard and a security center - COMPLETE; objective 2 find and destroy the hostile threat - COMPLETE), CONTINUE |
| ~100:03 | main menu (Interplay / Paramount notices, logo) |
| 100:06-100:19 | Single Player Campaign screen, Tutorial selected, mission list Tutorial / Building / Mining Resources / Power / Defense / Combat; end of file |

### 1a. C8 briefing (pages, views)

| Time | Page (paraphrase) | View |
|---|---|---|
| 55:42-56:01 | the last tutorial: build a working colony, make a defence force, beat a hostile threat | Federation emblem |
| 56:01-56:14 | the colony starts at a first stage; use what you learnt to build everything a viable colony needs | ground view over concrete, the hub/storage, pines (camera panning) |
| 56:14-56:20 | use scouts to find the enemy, build a defence force to fight it | ground view of the enemy base: dark pyramid tower, low brown buildings among pines (Klingon) |
| 56:20-56:33 | more colony information will come during the tutorial; good luck | emblem |

Pages are short (6-19 s); the voice-clip search finds none of tut-2_23-27
here (audio too degraded; see 8). Buttons NEXT / MISSION bottom right.

## 2. Timings

### Buildings (all Federation, C8)

| Building | Ordered (click) | Pad / bees | Done | Took |
|---|---|---|---|---|
| Construction yard | 56:55 (hologram green 56:52-56:54) | dark pad from 57:01, front part first; bees (tiny dark specks) round it | 57:41 (fully drawn; generic clip `B_fcycom` matched 57:40.9, weak 0.26) | **≈46 s** |
| Mining station 1 | 58:50 (green hologram on bare ground off the concrete) | 58:54-59:06 low pad | tower with ring standing ~59:46 | ≈56 s |
| Mining stations 2-4 | ~59:22-59:36, ~59:56, 61:04 (each a 140 Duranium drop) | | 2nd ~60:28, 3rd ~61:16-61:24 | ~50-60 s |
| Power generators 1, 2 | 61:37, 61:46 (stock drops; a red hologram at 61:44 when held on a bad spot) | | both standing by ~62:24 (two small towers with cyan rings beside the hub) | ≈40-45 s |
| Vehicle yard | 64:36 (green hologram on concrete W of hub) | footprint starts 64:46; dark pad painted 64:49-65:04, building revealed front to back 65:04-65:40 | 65:40 | **≈64 s** |
| Hub upgrade (Improved Hub) | panel open 57:58-58:28 (not bought); bought ~74:44-74:48 | | | |
| Construction yard upgrade (Improved Yard) | 75:40 | | | |
| Science station | exists by 77:36 (order not caught) | | | |
| Security center | menu tooltip 78:40-78:48 | | operational in panel 83:08 (viable-colony message 81:24 needs it, so done by 81:24) | |
| Vehicle yard upgrade (Specialist Yard) | hovered 82:28, again 88:00 | | | |
| Resource processor add-ons (Nitrium / Duranium plant) | list open 77:44, 83:16-83:20 | | | |
| Science labs (shield buffer, tracking systems, sensor, away team, energy) | lists open 77:40, 81:52-82:20, 83:00, 83:40 | | | |

Pattern: a small building ~45 s, construction yard 46 s, vehicle yard
64 s, mining station ~55 s at this colony's power (hub alone was at
40 % demand met until the generators came at ~62:30, so the first three
were built underpowered; no slowdown is obvious). Supports the 40 s
placeholder as an order of magnitude, with bigger buildings slower.

### Vehicles (vehicle yard)

| Unit | Ordered | Out | Time |
|---|---|---|---|
| Ground scout | 66:44 (ENGINEERING, scout tooltip) | 66:52 EN ROUTE, 66:56 READY at rally | **≈8 s** |
| Armed transport x5 | "5 ORDERED" 82:44 (one also at 77:28?) | completion-type clip `FED005b` at 82:57, 83:22, 83:48, 84:12, 84:36, 85:01 | **≈25 s each** |
| Phaser tank x4 | "4 ORDERED" 85:24 | `FED005b` at 86:02, 86:51, 87:41, 88:30 (+89:20, 90:10: more ordered) | **≈50 s each** |
| Photon artillery x2+ | 90:20-90:24 ("1", then "2 ORDERED") | `FED005b` 91:04, 91:54, 92:44, 93:34 | **≈50 s each** |

The `FED005b` hits (1.5 s, scores 0.29-0.45) are weak but strictly
periodic and line up with the vehicle yard's MANUFACTURING bar restarting
(e.g. 88:36 red, 89:28 full), so they are likely the "unit ready" voice.
Build time looks proportional to cost (scout 4 units of stock, transport
~9, tank/artillery ~30).

### Unit speed

Not measured cleanly (no Tricorder reading on a unit at both ends).
Rough: the 15-unit force (tanks + artillery + transports) was ordered from
the colony at ~94:12-94:16 (cursor readings <218:242> ... <072:349>,
"FRIENDLY UNIT" 94:10, "HOSTILE UNIT" 94:09) and reached the Klingon
base, top left of the full Tricorder, at ~95:24-95:36: ≈75-80 s for
about half the map's height (mixed group, moves at the slowest).

### Mining and materials

- Start stock (material storage, 56:34): Dilithium 300, Polycomp 1000,
  Thermoconc 2500, Trans Al 500, Nitrium 2000, Duranium 5000. Colony
  20/50 at the first INFO view (60:08).
- Stock rises from ~62:24 (after the generators; nothing was refined
  before that). Hub INFO readings:

| Time | Size | Dil | Poly | Thermo | TransAl | Nitrium | Duran |
|---|---|---|---|---|---|---|---|
| 61:28 | 21/50 | 300 | 926 | 1861 | 452 | 1950 | 4440 |
| 62:50 | 23/50 | 248 | 959 | 1804 | 466 | 497 | 4577 |
| 64:04 | 23/50 | 871 | 967 | 1832 | 494 | 762 | 4852 |
| 64:40 | 24/50 | 877 | 964 | 1748 | 495 | 929 | 4918 |
| 70:36 | 27/50 | 589 | 1064 | 1784 | 599 | 1960 | 5930 |
| 71:24 | 27/50 | 408 | 1077 | 1797 | 612 | 287 | 6067 |
| 72:20 | 29/50 | 382 | 1090 | 1776 | 625 | 1466 | 6205 |
| 75:00 | 30/50 | 486 | 1155 | 1757 | 662 | 1877 | 6747 |
| 98:48 | 35/50 | 1060 | 1504 | 2156 | 1227 | 2920 | 9090 |

  (Some digits small; 64:04's 871 Dilithium is as read.) Duranium grows
  steadily ~+140/min, Nitrium ~+150-250/min between purchases, Trans Al
  ~+20/min, Polycomp ~+15/min; Thermoconc barely moves.
- Mining station raw stock (70:40-71:20, one station): RAW DILITHIUM 6,
  TALGONITE 25, SILICON 5, KELBONITE 7, MAGNESITE 6, DOLAMIDE 9
  (unchanged for 40 s); at 85:32 another: 7 / 5 / 7 / 0 / 7 / 0.
- Resource processor raw stock: 77:48 Dil 312, Talg 413, Silicon 413,
  Kelb 92, Magn 122, Dol 89; 78:24 309 / 441 / 409 / 92 / 122 / 90;
  83:24 329 / 717 / 431 / 128 / 123 / 126. **Talgonite piles up**
  (413 to 717 in 5½ min) while the others stay put: the processor isn't
  using talgonite (or only slowly), so whatever talgonite makes isn't
  produced at the rate it arrives.
- Mine exhaustion (Engineering, bearded man): 65:27 at 060, 420 (7/7);
  67:02 at 060, 396 (9/9); 71:04 at 084, 436 (10/10); 71:49 at 116, 380
  (11/11); 72:47 at 092, 380 (12/12). (First digits blurred: 060 could be
  068.) So these mines lasted only ~6-13 minutes.

### Crew / colony size

20/50 (60:08), 21 (61:08), 22 (61:40), 23 (61:48), 24 (64:40), 26
(70:24), 27 (70:28), 28 (71:56), 29 (72:00), 30 (75:00), 35 (98:48). The
maximum stayed **50** throughout (nothing raised it: no transporter pad
or accommodation was built). Growth is irregular, ≈15 in 38 minutes
(much slower than the 30-s placeholder; it may be capped by assignment).
Hub POOL: 145 (57:46), 140 (63:28), 137 (66:06), 128 (74:52), 124
(80:16). The vehicle yard got CREW 5 between 65:44 and 66:08 after the
"insufficient crew" message (8/8).

### Power (hub panel)

| Time | Generator | Consumption | Demand met | Reserve |
|---|---|---|---|---|
| 57:46 | LOW | 100 % | 40 % | 0 % |
| 63:28 | LOW | 65 % | 100 % | 30 % |
| 63:32 / 63:36 | LOW | 65 % | 100 % | 37 % / 44 % |
| 66:06 | LOW | 70 % | 100 % | 100 % |
| 74:52, 78:28 | LOW | 75 % | 100 % | 100 % |
| 80:16 | LOW | ~94 % | 100 % | ~98 % |

Two power generators took demand met from 40 % to 100 %; reserve then
fills ≈1.75 %/s (30 % to 44 % in 8 s). Other buildings show CONSUMER /
SUPPLY 100 % / RESERVE (0 % before 62:30, 100 % after). The power
warning came at 61:33 and again at 62:33 (exactly 60 s apart).

## 3. Build menu and costs

Layout as in play-tutorials.md: a column of three models per page,
emblem top left, page arrows and X at the bottom, tooltip below. Read in
C8 (T, G, KG as shown):

| Item | Cost read | vs play-tutorials.md |
|---|---|---|
| Construction yard (62:32, 64:32, 81:40) | Poly 12T, Thermo 89T, TransAl 5T, Nitrium 50KG | same |
| Power generator (72:40) | Dil 35G (tooltip digit looks like 25/29; the stock drop at 61:37-61:40 is exactly 35), Poly 1T, Thermo 35T, Nitrium 800KG (stock 1950→1150) | same |
| Mining station | from stock drops: Poly 14, Thermo 80, TransAl 10, Duranium 140 | same |
| Vehicle yard | stock drop ~Thermo 90 at 64:36 | same |
| Sickbay (73:12) | Poly 8T, Thermo 48T, TransAl 20T | same |
| Security center (78:44) | Poly 10T, Thermo 100T, TransAl 10T, Nitrium 1200KG | same |
| Ground scout (66:44) | Dil 2G (or 3G; small), Poly 1T, TransAl 1T | Dil read 3G there |
| Armed transport (72:56, 82:40) | Dil 4G, Poly 3T, TransAl 1T, Duranium 30KG | same |
| Work bee (66:40) | TransAl 1T | same |
| **Phaser tank** ("phaser vehicle", 85:16-85:24) | **Dil 15G, Poly 12T (or 18T), TransAl 2T, Duranium 100KG** | new |
| **Photon artillery** ("mobile photon artillery", 90:20-90:24) | **Dil 10G, Poly 16T, Nitrium 100KG, Duranium 80KG** | new |
| Improved Hub (hub upgrade) | Poly 4T, Thermo 40T, Duranium 1000KG | new |
| Improved Yard (con yard upgrade) | Poly 2T, Thermo 20T, TransAl 4T | new |
| Specialist Yard (vehicle yard upgrade) | Poly 1T, Thermo 50T, TransAl 3T | new |
| Science station add-ons: shield buffer lab, tracking systems, sensor lab, away team lab, energy lab | each Poly 10T, Thermo 15T, TransAl 7T | new |
| Resource processor add-ons: dilithium, polycomp, thermoconc, trans al, nitrium, duranium plant | each Poly 2T, Thermo 12T, TransAl 10T (digits small) | new |

Upgrades/add-ons are shown in the building's own panel (title "<BUILDING>
UPGRADE" / "UPG"), as a list of icons with name and cost; science
station's bottom buttons are RAZE / RSRCH (research), others RAZE / BUILD.
The ENGINEERING menu shows "<n> ORDERED" in the tooltip after clicks
(queue). The vehicle yard panel shows MANUFACTURING <unit> with a
red/green progress bar.

The construction yard panel carries an extra row: the hub icon with
**COLONY SIZE n %**: 200 % (58:36), 250 % (64:00), 350 % (72:56), 375 %
(74:40), 450 % (80:12). Meaning unknown (stage/capacity?).

## 4. Placement

- Green hologram on free concrete (con yard 56:52, vehicle yard 64:36,
  generators 61:36). **Mining stations went on bare ground off the
  concrete** (58:50, 59:22: green hologram on grass/rock beside the pad,
  on deposits).
- A **red** hologram at 61:44 near the hub/storage (too close to a
  building) - not placed; moved and placed green.

## 5. Construction look

Vehicle yard 64:46-65:40 frame by frame: a dark (shadow-coloured) pad
grows over the footprint from one corner, the yellow-black hatch stripe
first, then the building's real texture is revealed in strips from the
front to the back; a few tiny dark specks (the construction bees) move
round it. Con yard the same (57:01-57:41). Mining stations rise as a
tower with a ring on top.

## 6. Controls

Nothing new taught in C8. Seen used: Tab full Tricorder, box/multiselect
(MULTISELECTION grid of up to 15 unit icons each with a health bar going
red when hit), STAFF on the hub with TRAINING button.

## 7. Map

Same temperate site as C5/C7: concrete pad on grass, pines, a lake and
a pale salt-like flat (94:48-96:36), a rocky dark patch. Tricorder
readings: scouting 67:04-68:20 <079:260>, <080:222>, <042:249>,
<041:402>, <041:277>, <052:462>, <055:458>, TALGONITE AT <052:458>,
IMPASSABLE between; 94:06-94:16 <218:242>, <215:244>, <222:244>,
<205:225>, <072:349>. Columns up to 462 again (past 64 blocks in eighths).
Mines 060-116 / 380-436 put the colony around rows 60-120, cols 330-440.

## 8. Messages (C8; no tutorial clip matched: audio too degraded)

| Time | n | Sender | Paraphrase |
|---|---|---|---|
| 56:34 | 1 | Command (young woman) | open COMMAND with the MISSION button to see objectives |
| 56:41 | 2 | Command | keep power up, mine more if needed |
| 56:49 | 3 | Command | scouts unarmed but quick spotters, tanks close combat, artillery long range only |
| 61:33 | 4 | Operations (young woman) | power too low, systems shutting down, build generators |
| 62:33 | 5 | Operations (woman) | same power warning, 60 s later |
| 62:42 | 6 | Operations (older grey-haired man) | construction delayed, not enough resources: build mines and a processor |
| 65:27 | 7 | Engineering (bearded man) | mine at 060, 420 exhausted |
| 65:44 | 8 | Operations (older man) | vehicle manufacture delayed: not enough crew; assign crew, build accommodation and a transporter pad |
| 67:02 | 9 | Engineering | mine at 060, 396 exhausted |
| 71:04 | 10 | Engineering | mine at 084, 436 exhausted |
| 71:49 | 11 | Engineering | mine at 116, 380 exhausted |
| 72:47 | 12 | Engineering | mine at 092, 380 exhausted |
| 81:24 | 13 | Command | viable colony established; power, resources and defence now up to you (objective 1) |
| 99:40 | 14 | Command | Klingon hub destroyed |
| 99:50 | 15 | Command | tutorial 5 done, all tutorials finished, ready for first training mission |

Message 8 ("insufficient crew") is new against play-tutorials.md. Weak
generic-clip hits: `FED002c` 62:43 (with message 6), `FED001c` 65:44
(with message 8), `katckvoc` 60:39 / 61:27, `fobjmet` 85:00, 86:50,
88:30, 91:54 (score ~0.26; doubtful - no objective met then).

## 9. Other

- STAFF (66:04): Ens. Bellah, Operations, basic; Lt. Dante, Engineering,
  highly skilled; Lt. Gage, Security, skilled - all in the colony hub.
- Hub panel bottom INFO / BUILD / STAFF; hub's vehicles WORKBEE 7,
  CARGOBEE 2, CNSTRCTNBEE 4, TRANSPORT 2 throughout.
- Building efficiency: material storage and con yard 40 % at start, 80 %
  later; hub 90 %.
- Combat 95:24-99:50: the force (15 icons at 93:52) meets the Klingon
  base (dark pyramid towers, low brown buildings on concrete); losses:
  15 → 10 by 95:48, 5 by 96:00, 3-4 left by 96:12-97:50; the player then
  builds more (ENGINEERING open 99:08-99:52) and photon/phaser fire
  destroys the hub at 99:40. Artillery shells: bright yellow-white bolts.

## 10. Answers to vm-checklist items

- **Build times** - con yard 46 s, vehicle yard 64 s, mining station
  ~55 s, power generator ~40-45 s; scout ~8 s, armed transport ~25 s,
  phaser tank / photon artillery ~50 s (sure ±5 s for buildings; vehicles
  from periodic clip hits, fairly sure).
- **Build costs** - phaser tank Dil 15, Poly 12 (18?), TransAl 2, Dur 100;
  photon artillery Dil 10, Poly 16, Nitrium 100, Dur 80; upgrades and
  science/processor add-ons as in 3 (sure for layout, digits ±1 glyph).
  Science block / phaser coil lab not seen.
- **Power** - two generators took the starting colony from 40 % demand
  met to 100 %; reserve fills ~1.75 %/s (sure).
- **Mining rates** - snapshots only; these C8 mines exhausted 6-13 min
  after building; processor talgonite stock grew 413→717 in 5½ min
  (medium).
- **Ores to materials** - partial: talgonite accumulates unused in the
  processor (medium).
- **Colony capacity growth** - 50 all mission long with no transporter pad
  / accommodation built (sure); crew 20→35 in 38 min.
- **Building on deposits** - not tested; mining stations placed off the
  concrete on deposits.
- **Starting stock and size** - C8: 300 / 1000 / 2500 / 500 / 2000 /
  5000, 20/50 (sure).
- **Panels never opened** - C8's MISSION REPORT text and C7's (partly) seen.
- **C7, C8** briefing views: C8 page 65 (here 56:01) is a ground view of
  the colony site; page 66 the Klingon base.
- **C4-C8** clips: none matched in C8 (audio).

## Still open

- Unit speed (no clean Tricorder start/end on one unit).
- When the resource processor and science station were built; which
  building the 70:24 tooltip (arched grid model) was; what spent 180
  Dilithium and 1670 Nitrium between 70:36 and 71:24 (two more generators?).
- Hub stage 2/3 and yard stage 3 exact times (needed by 81:24).
- The con yard's "COLONY SIZE n %" row.
- Scout cost digit (2 or 3), phaser tank Polycomp (12 or 18).
