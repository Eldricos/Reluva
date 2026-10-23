# Star Trek: New Worlds: the training missions recorded in a VM - C7 Defense

From the project's own recording of the game played in a virtual machine,
"Tutorials - Others", 32:55-55:33. The game picture is the 640 x 480 game scaled to 712 x 534; the
sound is poor, so some lines are read from the screen. Times are mm:ss in
that video file. Messages and pages are paraphrased. Summary and how this
adds to the earlier review of another player's recording:
[play-vm-tutorials.md](play-vm-tutorials.md) and
[play-tutorials.md](play-tutorials.md).

## 1. What the stretch covers

| Time | What |
|---|---|
| 32:19-32:47 | end of C6 (the previous stretch): message 4/4 at ~32:20 says nitrium now allows a **fifth power generator**, build it to finish; `B_fpgcom` (generator done, score 0.23) at 32:39.4; 5/5 at 32:39 says all the required generators run; 6/6 at 32:47 says tutorial 3 is complete and tutorial 4 is loading. `fmiswin` 32:57.4 (0.35), `fobjmet` 32:59 |
| 32:57-33:03 | **second Federation emblem screen** (plain emblem on black, no loading box seen), 33:04-33:05 black |
| 33:06-33:58 | C7 briefing, four INSTRUCTION pages (below) |
| 33:59-34:00 | emblem again, 34:01-34:02 black |
| 34:03 | C7 in play |
| 55:23 | message 14/14: tutorial 4 done, tutorial 5 loading |
| 55:31-55:32 | black; **third emblem screen 55:33-55:39** (C8 loading), black 55:40-55:41, C8 briefing 55:42-56:30, C8 play from 56:33 |

So the third emblem is at **55:33**, not ~61:30. The brightness trace
 has no
other black/emblem break up to 63:44.
C7 ran 34:03-55:23, about 21 minutes. It was completed and chained into
C8 without going through the menu.

### C7 briefing (pages 48-51)

| Shown | Page (paraphrase) | View |
|---|---|---|
| 33:06-33:21 | this tutorial teaches building a defence force against attacks | Federation emblem |
| 33:22-33:35 | photon systems need a security center, a level-2 building that needs a stage 2 hub and a stage 2 construction yard | **blue wireframe of the security center** on the grid (long low block with a raised middle, green-black stripes at one end) |
| 33:36-33:48 | phaser tanks also need a stage 2 vehicle yard and a science block with the phaser coil lab | blue model of the vehicle yard (arched hangar) |
| 33:49-33:58 | keep the power up, add mines if needed, good luck | first-person ground run across the colony concrete (hub, storage, hangar) |

Each page lasts 10-16 s. No `tut-2_18`-`21` clip (or any other tutorial
clip) matches here (best 0.12-0.23, envelope correlation too). The
prep's `matches.txt` for t2 finds tutorial clips only up to 9:13. Past
that point the audio is too poor to match: generic clips score only
0.2-0.4. So the C7 clips can't be confirmed from this recording.

### Objectives (COMMAND panel)

Opened at 34:15-34:21 (all INCOMPLETE), 43:53-43:59, 46:18-46:23, 47:38-47:45 and 48:40-48:41.
The header reads DEFENSE / TUTORIAL 4: DEFENDING A COLONY. There are three objectives:
- build a security center. COMPLETE by 46:21 (built 45:39)
- give the colony 2 phaser tanks. INCOMPLETE through 48:41
- give the colony 2 photon artillery units. COMPLETE by 48:41

## 2. Timeline of C7 play

### Messages

| Time | n | Section (portrait) | Paraphrase | Clip (score) |
|---|---|---|---|---|
| 34:05 | 1/1 | Command (young woman) | open COMMAND with the MISSION button to see the objectives | none |
| 34:11 | 2/2 | Command | the security center needs the hub and the construction yard at stage 2 | none |
| 35:52 | 3/3 | Command | photon artillery now needs the vehicle yard at stage 2, which needs stage 2 hub and yard (comes as the new vehicle yard is finished) | `B_fvycom` 35:52.6 (0.20) |
| 39:03 | 4/4 | Operations (same young woman) | colony power too low, systems shutting down, build generators | `flowpowr` 39:03.4 (0.15) |
| 41:34 | 5/5 | Command | photon units are available; phaser tanks need a science block upgraded with a phaser coil lab | `B_fvycom` 41:34.9 (0.26): vehicle yard stage 2 done |
| 43:06 | 6/6 | Engineering (bearded man) | the mine at **060, 396** is exhausted | none |
| 45:39 | 7/7 | Command | security center working; build a vehicle yard for mobile units | `B_fsccom` 45:39.4 (0.18) |
| 46:45 | 8/8 | Engineering | the mine at **060, 420** is exhausted | (`fobjmet` 46:47 0.26, doubtful) |
| 47:21 | 9/9 | Operations | power too low again (generators) | `fobjmet` 47:20.8 (0.20)? |
| 48:34 | 10/10 | Command | photon artillery vehicles complete | `fobjmet` 48:34.3 (0.20) |
| 49:20 | 11/11 | Engineering | the mine at **092, 300** is exhausted | none |
| 55:15 | 12/12 | Command | phaser tanks complete | `fobjmet` 55:14.9 (0.22) |
| 55:19 | 13/13 | Command | the defence force the colony needs is now there | none |
| 55:23 | 14/14 | Command | tutorial 4 done, tutorial 5 loading | `fobjmet` 55:33.5 (0.16) |

This corrects play-tutorials.md's C7 list:
- In this run the security center is done at 45:39 (message 7), not near the end.
- Here "Operations" uses the young woman's portrait, as in C1.
- No "construction delayed" message comes in this run.

### Buildings and upgrades (order = the click on the green hologram or upgrade icon)

| Item | Ordered | Pad / bees | Finished | Took | Evidence |
|---|---|---|---|---|---|
| Vehicle yard (2nd one, left of the hub) | 34:46 (green hangar hologram 34:44-34:46) | dark plates 34:56 | ~35:52 | ~66 s | `B_fvycom` 35:52.6 + message 3 |
| Mining station 1 (silicon, ~<057:408>, off the concrete on grass) | 35:20 (red hologram at 35:03 on plain grass, green at 35:19) | rising 36:10 | 36:40 | 80 s | `B_fmscom` 36:40.3 (0.17) |
| Mining station (magnesite, ~<094:2..>) placed **in the full-screen Tricorder** | ~36:52-36:59 | not watched | 37:26? | ~30 s? | `B_fmscom` 37:26.8 (0.24); unsure which mine |
| Mining station 2 (on grass near mine 1) | 37:07 (green 37:04-37:07) | rising 37:35 | ~38:14 | ~67 s | `B_fmscom` 38:14.5 (0.23) |
| Power generator 1 | 38:22 (red 38:21, green 38:22) | first bee 38:28, plates 38:30-38:36 | top lit 38:44, bees gone ~38:52 | ~28 s | DEMAND MET 14 to 22 % at 38:54-38:56 |
| Colony hub upgrade 1 (IMPROVED HUB) | 38:44 | pods start 38:54 | ~39:30 (four round pods) | ~45 s | DEMAND MET 22 to 24 % by 39:33 |
| Power generator 2 | 39:20 (green 39:19) | bees 39:25, plates 39:28 | top lit 39:46, bees gone 39:52 | ~32 s | DEMAND MET 24 to 54 % at 39:54, 62 % at 39:59 |
| Construction yard upgrade 1 (IMPROVED YARD) | ~40:02-40:10 | bees 40:14-40:26 | ~40:30 | ~25 s | `B_fcycom` 40:32.9 (0.15); rounded front afterwards |
| Vehicle yard upgrade 1 (EXTENDED YARD) | 40:47 | dark plates on the hangar 41:10 | 41:35 | ~48 s | `B_fvycom` 41:34.9 + message 5 |
| Science station | 42:24 (red 42:23 against the storage, green 42:24) | plates 42:35, walls 42:45 | ~43:40 (top ring closed) | ~76 s | panel at 43:44, EFFICIENCY 55 % |
| Security center | 44:16 (hover/click 44:14, green 44:16, top of the concrete) | rising 44:55 | 45:39 | 83 s | `B_fsccom` + message 7 |
| Power generator 3 | 47:28 (red 47:27 beside parked vehicles, green 47:28) | | ~48:06 | ~38 s | `B_fpgcom` 48:05.9 (0.19); DEMAND MET 92 to 100 % at 48:08-48:12 |
| Colony hub upgrade 2 (ADVANCED HUB) | 48:51 | plates 49:00-49:20 | ~49:45 (eight-sided dome) | ~54 s | frames |
| Construction yard upgrade 2 (ADVANCED YARD) | 50:09 | | ~50:20-50:25 | ~15 s | frames (dome-topped yard by 50:20) |
| Vehicle yard upgrade 2 (SPECIALIST YARD) | 50:43 | plates 50:50-51:15 | 51:30 | ~47 s | `B_fvycom` 51:30.3 (0.26) |
| Science labs: sensor lab, tracking systems, shield buffer lab | 52:13-52:20 (the three icons clicked in turn) | | 52:38, 52:47, 52:54 | ~25-35 s each | three `B_fsbcom` (0.23/0.19/0.23) |
| Science lab: phaser coil lab | 53:05 | | ~53:26 | ~21 s | `B_fsbcom` 53:26.3 (0.13); phaser vehicle in the menu at 53:34 |

Reading the `com` clips: the clip is the building's spoken name. The
matches line up with the finishes seen in the frames, and
`B_fsbcom` = science block. No build menu was open at those moments, so
these are completions.

### Vehicles (vehicle yard)

| Vehicle | Ordered | Done | Took |
|---|---|---|---|
| Photon artillery 1 | 46:07 (tooltip turns to "…I ORDERED") | between 46:40 and 47:47 (panel shows MANUFACTURING PHOTON ARTILLERY with a green-red bar 46:18-46:39, gone at 47:47) | ≤60 s |
| Photon artillery 2 | 47:54 | 48:34 (message 10) | ~40 s |
| Phaser vehicle 1 | 53:34-53:45 ("I ORDERED" at 53:45) | ~54:30 (the bar restarts small and red by 54:38) | ~50 s |
| Phaser vehicle 2 | queued | 55:15 (message 12) | ~40 s |

The panel shows one item at a time with a progress bar (green part grows
over red). Unit speed: no unit was sent across the map in this stretch.
**Not measured.**

## 3. Build menu (costs from hover tooltips)

Structures are three to a page. The pages scroll:
- page 1: storage facility, power generator, construction yard
- page 2: vehicle yard, resource processor, mining station
- page 3: transporter pad, science station, sickbay
- page 4: security center, alone on its page (seen from 44:14, once hub and yard were at stage 2)

Hovering an icon shows the name, a one-line role, then the costs. A clicked icon turns magenta.

| Item | Cost shown | Time |
|---|---|---|
| Vehicle yard | Polycomp 10T, Thermoconc 90T, Trans Al 5T | 34:41 |
| Construction yard | Polycomp 12T, Thermoconc 89T, Trans Al 5T, Nitrium 50KG | 35:00 |
| Mining station | Polycomp 14T, Thermoconc 80T, Trans Al 10T, Duranium 140KG | 37:03 |
| Power generator | **Dilithium 25G**, Polycomp 1T, Thermoconc 35T, Nitrium 800KG | 39:17, 44:04 |
| Storage facility | supplied pre-built, no materials | 42:04 |
| Resource processor | Polycomp 4T, Thermoconc 30T, Trans Al 2T | 42:06 |
| Sickbay | Polycomp 8T, Thermoconc 48T, Trans Al 20T | 42:10 |
| Science station | Polycomp 4T, Thermoconc 53T, Trans Al 17T, Nitrium 120KG | 42:11 |
| Transporter pad | Polycomp 6T, Thermoconc 32T, Trans Al 2T | 42:12 |
| Security center | Polycomp 10T, Thermoconc 100T, Trans Al 10T, Nitrium 1200KG | 44:14, 48:27 |

Vehicle menu (ENGINEERING), three to a page:
- page 1: cargo bee, construction bee, work bee
- page 2: ground scout, armed transport, photon artillery
- later: an ADV. ARTILLERY page (51:36, after the vehicle yard reached stage 3) and a PHASER VEHICLE icon (53:34, after the phaser coil lab)

| Vehicle | Cost shown | Time |
|---|---|---|
| Construction bee | Dilithium 1G, Polycomp 1T, Trans Al 1T | 41:43 |
| Ground scout | **Dilithium 2G**, Polycomp 1T, Trans Al 1T | 41:48 |
| Armed transport | Dilithium 4G, Polycomp 3T, Trans Al 1T, Duranium 30KG | 41:49 |
| Work bee | Trans Al 1T | 46:30 |
| Cargo bee | Dilithium 1G, Polycomp 1T, Trans Al 1T | 46:37 |
| **Photon artillery** (mobile photon artillery) | Dilithium 10G, Polycomp 16T, Nitrium 100KG, Duranium 60KG | 46:00 |
| **Adv. artillery** (Federation advanced photon tank) | Dilithium 12G, Polycomp 18T, Nitrium 150KG, Duranium 90KG (~) | 51:36 |
| **Phaser vehicle** (Federation phaser tank) | Dilithium 15G, Polycomp 19T (~18), Trans Al 2T, Duranium 100KG | 53:34 |

Upgrades (costs on the building's next page):

| Upgrade | Cost |
|---|---|
| Colony hub to IMPROVED HUB | Polycomp 4T, Thermoconc 40T, Duranium 1000KG |
| Colony hub to ADVANCED HUB | Polycomp 50T, Thermoconc 20T, Trans Al 20T |
| Construction yard to IMPROVED YARD | Polycomp 2T, Thermoconc 20T, Trans Al 4T |
| Construction yard to ADVANCED YARD | Polycomp 20T, Thermoconc 10T, Trans Al 6T |
| Vehicle yard to EXTENDED YARD | Polycomp 1T, Thermoconc 50T, Trans Al 3T |
| Vehicle yard to SPECIALIST YARD | Polycomp 1T, Thermoconc 50T, Trans Al 3T |
| Science station labs (each) | Polycomp 10T, Thermoconc 15T, Trans Al 7T |
| Resource processor plants (each) | Polycomp 2T, Thermoconc 12T, Trans Al 10T |

The science station's lab list:
- 43:46: shield buffer lab and sensor lab only.
- 52:06, after the hub, yard and vehicle yard reached stage 3: tracking systems is added.
- Once those three were bought: phaser coil lab, energy lab and away team lab.

The resource processor's list:
- 47:10: dilithium, polycomp, thermoconc and trans al plants.
- 52:02: a NITRIUM PLANT is added.

None of the processor plants was bought.

The stock bears out the costs. Three mines take 240 thermoconc and 420
duranium: 2210 to 1970 and 5000 to 4580 by 37:09. Two generators take 50
dilithium: 290 to 239 by 40:00. The security center takes 1200 nitrium:
1826 at 44:16 to ~626, and 767 shows at 45:00.

## 4. Placement

- **Mines go off the concrete.** The hologram is red over plain grass
  (35:03) and green over a deposit (35:19, Tricorder SILICON AT); both
  mines in the main view stand on grass among pines. A mine can also be
  placed from the full-screen Tricorder (36:52-36:59, cursor over a
  MAGNESITE ORE AT block, a small green outline).
- **On the concrete**, the hologram turns red right next to a building
  (science station against the storage, 42:23) or beside parked
  vehicles (generator 3, 47:27). One cursor step away it turns green.
- I didn't see a generator held over a deposit. The "Building on
  deposits" item stays open.

## 5. Construction look

There is no ready-made pad:
1. Within ~6 s of the click, 3-4 small dark construction bees fly in.
2. They lay dark plates that grow into the building's footprint (generator 38:30-38:36).
3. The pink walls rise from the plates.
4. The top goes on last (the generator's cyan ring 38:44, the science station's ring 43:39).
5. The bees fly off.

Upgrades work the same way: dark plates are laid over the old building
and the new shape grows out of them:
- hub stage 2: four round pods
- hub stage 3: a big eight-sided dome
- construction yard stage 2: a round-fronted hatch
- vehicle yard upgrades: plates over the hangar

## 6. Controls (only what is new)

- Tab's full-screen Tricorder accepts build placement (mine at ~36:55).
- Its top line is the hover readout. Examples:
  - TERRAIN AT <045:402>
  - SILICON AT <056:406>, <057:408>, <059:399>, <060:399>
  - TERRAIN AT <094:248>(~)
  - MAGNESITE ORE AT <090:2..>, <094:2..> (cut off)
  - IMPASSABLE
  - the small Tricorder header shows TALGONITE AT <03?:3??> at 36:07-37:05
- At this size the small Tricorder header digits can't be read.

## 7. Map

- The site is the same temperate concrete square among pines and
  grassland as C5/C6.
- Deposits lie south of the colony around rows 45-95, columns 220-410,
  with silicon at ~<057:408>.
- Exhausted mines: 060,396 / 060,420 / 092,300. These match play-tutorials.md's C7/C8 area (rows 46-172).
- The colony hub's own position was not read (digits too small).
- The compass strip reads ---350---+---[ N ]---+---010---.

## 8. Mining and materials

Resource processor raw stock (RAW DILITHIUM, TALGONITE, SILICON, KELBONITE, MAGNESITE, DOLAMIDE):

| Time | Raw stock |
|---|---|
| 37:23-37:30 | 0 / 0 / 0 / 0 / 0 / 0 (mine 1 working since 36:40) |
| 37:50 | 13 / 2 / 2 / 2 / 2 / 2 |
| 38:10 | 36 / 6 / 6 / 6 / 6 / 6 |
| 38:18 | 49 / 9 / 9 / 9 / 9 / 9 |
| 38:30 | 60 / 10 / 10 / 10 / 10 / 10 |
| 38:40 | 72 / 12 / 12 / 12 / 12 / 12 |
| 47:09 | 312 / 102 / 208 / 34 / 24 / 37 |
| 49:15 | 331 / 129 / 276 / 81(~) / 92 / 25(~) |
| 51:58 | 367 / 221 / 510 / 59 / 100 / 59 |

- The first ore arrives about 70 s after mine 1 finishes.
- From then raw dilithium rises ~1.1 a second, each other ore ~0.2 a second, all at once.
- The stock keeps piling up: the processor refines more slowly than ore comes in.
- Every ore arrives even though mine 1 sits on a silicon block.
- Mine lifetimes:
  - mine 1: 36:40 to 43:06 (6.4 min)
  - mine 2: ~38:14 to 46:45 (8.5 min)
  - the magnesite mine (if 37:26): to 49:20 (11.9 min)

Colony materials (INFO / yard panels; Dilithium, Polycomp, Thermoconc, Trans Al, Nitrium, Duranium):

| Time | Stock |
|---|---|
| 35:05 | 290 / 990 / 2210 / 495 / 2600 / 5000 |
| 37:09 | 290 / ~948 / 1970 / ~465 / 2600 / 4580 |
| 39:12 | 255 / 945 / 1895 / 465 / 1900 / 3580(~) |
| 39:32 | 255 / 955 / 1875 / 479 / ~1127 / 3717 |
| 40:44 | 247 / 967 / 1857 / 488 / 1275 / 3855 |
| 41:39 | 274 / 995 / 1854 / 512 / 1542 / 4122 |
| 42:21 | 287 / 1007 / 1848 / 526 / 1690 / 4250 |
| 43:42 | 315 / 1050 / 1822 / 526 / 1826 / 4535 |
| 45:00 | 328 / 1054 / 1736 / 540 / 767 / 4667 |
| 45:47 | 342 / 1047 / 1749 / 553 / 902 / 4802 |
| 48:45 | 355 / 1052 / 1782 / 621 / 592 / 5322 |
| 49:45 | 368 / 1046 / 1766 / 615 / 720 / 5460 |
| 51:09 | 396 / 1052 / 1733 / 622 / 996 / 5795 |
| 54:18 | 435 / 1043 / 1727 / 657 / 1537(~) / 6177 |

Production between purchases:
- duranium ~2.5-3.5 a second
- nitrium ~3-5 a second
- dilithium ~0.3 a second
- trans al ~0.1 a second
- polycomp and thermoconc flat (only spent)

With the raw stock dominated by dilithium and silicon, it looks as if
the plain processor turns ore into nitrium and duranium mostly, and
the per-material "plants" would open the other lines. That is a guess.
Which ore feeds which material can't be told from this run.

## 9. Power (colony hub panel; GENERATOR, CONSUMPTION, DEMAND MET, RESERVE)

| Time | Readings | Event |
|---|---|---|
| 38:42-38:53 | LOW, 100 %, 14 %, 0 % | no generator yet; message 4 at 39:03 |
| 38:56-39:00 | LOW, 100 %, 22 %, 0 % | generator 1 done |
| 39:33-39:51 | LOW, 100 %, 24 %, 0 % | hub stage 2 done |
| 39:54 / 39:59 | 54 % / 62 % | generator 2 done |
| 48:08 | LOW, 99 %, 92 %, 0 % | |
| 48:12-48:16 | LOW, 99 %, 100 %, 0 % | generator 3 done ~48:06 |
| 48:48 / 48:49 / 48:53 / 49:04 | 81 %, 100 %, reserve 25 / 27 / 32 / 47 % | consumption fell 99 to 81 % between 48:16 and 48:48, around the second artillery finishing |
| 51:56 | LOW, 81 %, 100 %, 100 % | |

- The reserve fills about 1.4 % a second once demand is met.
- The hub GENERATOR stays LOW throughout. The hub upgrades don't change it.
- A power generator's panel (50:05): GENERATOR MED, CONSUMPTION 81 %, DEMAND MET 100 %, RESERVE 100 %, EFFICIENCY 120 %.
- Other buildings show CONSUMER / SUPPLY 100 % / RESERVE 100 % once power is met.
  - The construction yard showed SUPPLY 100 %, RESERVE 0 %, EFFICIENCY 40 % at 35:05, then 80 % efficiency from 40:05.
  - The science station showed SUPPLY 125 %, RESERVE 22 %, EFFICIENCY 55 % at 43:44, then 80 % by 47:04.

C7 needed three generators (two at the start, one after message 9).
C6 needed five: its message 4/4 asks for a fifth.

## 10. Crew and colony size

- INFO COLONY SIZE:

  | Time | Size |
  |---|---|
  | 39:12 | 24/50 |
  | 39:32-42:21 | 25/50 |
  | 43:00-44:16 | 26/50 |
  | 45:00-45:45 | 27/50 |
  | 48:45-51:09 | 30/50 |

  That is one person every ~1.5-2 min. The maximum stayed **50** through
  both hub upgrades, three generators, a science station and a security
  center. None of these raised it.
- The construction yard panel has its own **COLONY SIZE nnn %** line:

  | Time | Value |
  |---|---|
  | 35:05 | 100 % |
  | 37:09 | 137 % |
  | 40:00-40:42 | 162 % |
  | 49:08-50:31 | 225 % |

  What it measures is unknown. It rises with buildings and upgrades
  (after the three mines; after the generators and hub stage 2; after
  hub stage 3), not with the crew.
- Hub staff: OFFICERS 5, CREW 0, POOL 128 (38:42), 126 (48:10).
- Hub vehicle counts: WORKBEE 9, CARGOBEE 2, CNSTRCTNBEE 4, TRANSPORT 2.
- Each finished building takes CREW 1 (vehicle yard CREW 5).

## Answers to vm-checklist items

- **Build times**: Federation, from the order click to finished:

  | Item | Time |
  |---|---|
  | power generator | 28-38 s |
  | construction yard upgrades | 15-25 s |
  | hub upgrades | ~45-54 s |
  | vehicle yard (new) | ~66 s |
  | vehicle yard upgrades | ~47-48 s |
  | mining station | 67-80 s |
  | science station | ~76 s |
  | security center | 83 s |
  | science labs | ~21-35 s |
  | photon artillery | ~40 s |
  | phaser tank | ~40-50 s |

  Several builds overlapped, with 4 construction bees, and power was short until ~48:12. Sure to ±3 s on frames; the `com` clips are weak matches.
- **Build costs**: phaser tank (Phaser vehicle), photon artillery and adv. artillery now have costs (above), plus every upgrade. The science block / phaser coil lab costs: each lab Polycomp 10T, Thermoconc 15T, Trans Al 7T (the "science block" is the science station). Sure, except a few blurred digits marked ~.
- **Power**: readings above. C7 needs 3 generators, C6 needs 5. Each generator adds roughly 8-38 points of DEMAND MET at 100 % consumption. Fairly sure.
- **Colony capacity growth**: in C7 the maximum stays 50 whatever is built (hub stages 2 and 3 included). Crew grows ~1 per 1.5-2 min. Sure.
- **Mining rates**: processor intake ~1.1 raw dilithium + ~0.2 of each other ore a second from one mine. Mines last 6.4-12 min. Fairly sure.
- **Ores to materials**: not settled; see section 8. Partial.
- **Unit speeds**: not measured in this stretch.
- **Building on deposits**: mines need deposit blocks and may stand off the concrete. Other buildings red next to buildings or units. No generator-over-deposit test. Partial.
- **Tricorder mineral rule**: in the full-screen Tricorder the hover line names the ore (SILICON AT, MAGNESITE ORE AT) over deposit blocks and says TERRAIN AT elsewhere. Main-view hover not tested.
- **Panels never opened** (COMMAND): the tutorial's COMMAND list was seen at start, midway and near the end (section 1).
- **C4-C8**: no tutorial clip matched in C7 (briefing or play). The audio is too degraded past 9:13 in this file. tut-2_05-07, 12, 17, 22, 27 and tutor40/41 were not found here either.
- **C6**: completes. A fifth generator is asked for (message 4/4), 5/5 says all generators run, 6/6 at 32:47 says tutorial 3 is complete. `fmiswin` at 32:57. (Seen at the start of this stretch.)
- **C7, C8**: briefing view for C7 page 49 is the security center wireframe.
- **Placeholders**:
  - Build time 40 s: actual 15-83 s by type.
  - Crew growth one every 30 s: here ~one every 100 s in C7.
  - Capacity +5 a building: not in C7.
  - Mining 1.5 ore a second: one mine gives ~1.9 ore a second into the processor.

## Corrections to play-tutorials.md

- Power generator costs **Dilithium 25G**, not 35G (and two generators took 51 dilithium).
- Ground scout Dilithium reads **2G** here (play-tutorials says 3G; blurry, check).
- C6 was completed (in this run) with five generators.
- C7 message order and times differ from the YouTube run: the security center was done 7th of 14 messages. In this run, "Operations" uses the young woman's portrait.
- Hub upgrades don't raise the colony maximum in C7 (50 throughout).

## Still open

- Which mine finished at 37:26 (the one placed from the Tricorder?), and whether the magnesite mine is the 092,300 one.
- The construction yard panel's COLONY SIZE % line: what it measures.
- Ore-to-material mapping; whether processor plants change it.
- When photon artillery 1 came out of the hatch (between 46:40 and 47:47).
- Unit speed; the hub's Tricorder position; voice clips for C7.
