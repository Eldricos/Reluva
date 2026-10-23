# Star Trek: New Worlds: the training missions recorded in a VM - C1 "Tutorial"

From the project's own recording of the game played in a virtual machine,
"Tutorial - Tutorial" (17:02). The game picture is the 640 x 480 game scaled to 712 x 534; the
sound is poor, so some lines are read from the screen. Times are mm:ss in
that video file. Messages and pages are paraphrased. Summary and how this
adds to the earlier review of another player's recording:
[play-vm-tutorials.md](play-vm-tutorials.md) and
[play-tutorials.md](play-tutorials.md).

## 1. Timeline

| Time | What |
|---|---|
| 0:00-0:03 | main menu (logo, version text box, six buttons) |
| 0:04-0:08 | **Options** opened: Volume Control (Sound Effects, Music, Voice sliders; music at ~60 %, the others full), **Screen Size** buttons 640 X 480 / 800 X 600 / 1024 X 768, Accept Settings / Cancel Settings; left with Cancel |
| 0:10-0:15 | **About**: a credits box replaces the version text (developer and producer names) |
| 0:16-0:21 | Single Player Campaign: Race and Difficulty (Average / Expert / Novice labels over Federation / Romulan / Klingon), Tutorial button, Cadet Mode On (Easy Game) unticked, planet picture, Select Mission list Tutorial / Building / Mining Resources / Power / Defense / Combat; Tutorial picked, Play |
| 0:22-0:25 | black |
| 0:26-0:34 | loading box over the three emblems: a stardate/index line, then lines loading interface parameters, aural subroutines, surface data, geo-survey data, and a Done n% counter |
| 0:35-0:38 | Federation emblem |
| 0:39-2:18 | briefing, Tutor01-07 (table below) |
| 2:19-2:20 | Federation emblem again |
| 2:21-2:22 | a second short loading box |
| 2:23 | in play; first message 2:24 |
| 2:23-16:38 | **C1** played, 45 messages (all Operations) |
| 16:34.6 | `fobjmet` (objective met) sound |
| 16:38 | message 45: the attackers are destroyed, training complete |
| 16:45-16:47 | loading box: initialising debriefing, TUTORIAL, officer command protocols, Done n% |
| 16:48-16:51 | Federation emblem (`fmiswin` matched weakly at 16:48.7) |
| 16:52-16:56 | **MISSION REPORT** over the emblem: MISSION LOG, then one objective: destroy attacking forces - COMPLETE (cvr03 id 16) |
| 16:57-16:58 | black |
| 16:59-17:02 | back at the main menu |

So C1 does end on its own after the attack is beaten. Its report lists only
objective 16, none of ids 10-15 (scout to 114, 255, artefact, processor,
mine, phaser tanks), and COMMAND was never opened.

### Briefing (clip start, view)

| Clip | Starts | View |
|---|---|---|
| Tutor01 (23.8 s) | 0:40.1 | galaxy, red/blue nebula band |
| Tutor02 | 1:04.4 | ground view of the colony (hub, storage, dome) |
| Tutor03 | 1:17.0 | the storage block close, hazard stripes |
| Tutor04 | 1:29.5 | colony seen across the concrete, then a flight low over hills and pines |
| Tutor05 | 1:42.0 | a ground scout on pinkish rocky ground, pines |
| Tutor06 | 1:54.6 | full-screen Tricorder relief, colony blue, deposits crosshatched |
| Tutor07 | 2:07.2 | translucent green hull turning on a dark grid: a long low hull with a raised cab at one end, more like a ground vehicle (armed transport?) than a starship; not identified |

The INSTRUCTION box sits top left or bottom left depending on the view.
Clips follow each other about 12.5 s apart, so a page lasts its clip plus a gap.

## 2. Starting colony and stock

- Buildings at the start (2:23): colony hub (middle), storage facility
  (west), vehicle yard (south, the yellow-black hatch facing north), a
  dome building in the north-east (it is in the hub's JUMP TO list, not
  selected by name), a square patch of bare churned ground east of the
  vehicle yard, small round units parked on the concrete. The concrete is
  an irregular cross of pads on grass, with mesas, pines and a lake to the
  north-east.
- Hub panel (2:55): GENERATOR LOW, CONSUMPTION 87 %, DEMAND MET 100 %,
  RESERVE 17 %; OFFICERS 4, CREW 0, POOL 124 (3:30), later 122 (6:20,
  11:56); WORKBEE 6, CARGOBEE 2, CNSTRCTNBEE 4, TRANSPORT 4. SHIELD COVER
  HIGH, EFFICIENCY 90 %.
- Hub INFO page (2:58): **COLONY SIZE 24/60**; Dilithium 660, Polycomp 160
  (3 digits), Thermoconc 2300, Transalumin 660, Nitrium 5600, Duranium
  6600. **Not** the 500/1000/2300/500/4000/5000 of C4 and the campaigns.
  JUMP TO grid: hub, a tower, a base with yellow trim, a two-part
  building, a vehicle (5 icons in one column).
- Storage facility (3:10-3:25): CONSUMER, SUPPLY 100 %, RESERVE 0 %;
  OFFICERS 0, CREW 1; the six materials (the same numbers as INFO);
  EFFICIENCY 80 %; only a BUILD button.
- At 9:50 (INFO again): **COLONY SIZE 26/60**; 657 / 145 (or 143) / 2171 /
  654 / 5550 / 5600.
- The difference 2:58 to 9:50 is exactly construction yard + hub upgrade +
  one ground scout: Dilithium -3 (scout), Thermoconc -129 (89 + 40),
  Trans Al -6 (5 + 1), Nitrium -50 (yard), Duranium -1000 (hub upgrade),
  Polycomp -15/-17 (12 + 4 + 1 = 17; the digits are tiny). Confirms the
  costs below and that the stock is spent once, at the order.

## 3. Timings

| What | Ordered | Pad / bees | Done |
|---|---|---|---|
| Construction yard (north of the hub) | 3:32.5 (green hologram 3:31-3:32, gone 3:33) | bees fly in 3:39-3:41; dark pad pieces 3:40; yellow-black apron 3:43 | building textured 4:06-4:18; `B_fcycom` voice 4:16.8 |
| Hub upgrade (improved hub) | 5:38 (tooltip 5:37.5, panel back on hub 5:38.5) | first dark piece 5:43 | three round pods added; all textured 6:16; `FED004b` 6:18.7 |
| Ground scout (vehicle yard) | 4:51.5 (click flash; the tooltip then reads GROUND SCOUT, 1 ORDERED) | - | not seen leaving the hatch up to 5:14, when the camera went back to the hub |

So build times were about **44 s** (construction yard, order to the
complete voice) and **~40 s** (hub upgrade, order to last pod textured), on
a hub at GENERATOR LOW with DEMAND MET 100 %. That agrees with the
placeholder 40 s.

## 4. Build menu and costs (hover tooltips)

- Hub BUILD (3:27-3:31): the NEW WORLDS panel turns into the build page:
  emblem top left, one big model icon a row (here only the **construction
  yard**: C1 offers nothing else), arrows and X at the bottom, the tooltip
  text at the panel's foot: name, a line of role, the costs.
  Construction yard: Polycomp 12T, Thermoconc 89T, Trans Al 5T, Nitrium
  50KG (as play-tutorials.md).
- Hub upgrade page (5:37): title COLONY HUB UPGRADE, the hub's status block
  on top, then the IMPROVED HUB icon: Polycomp 4T, Thermoconc 40T,
  Duranium 1000KG. **New** (play-tutorials.md has no hub upgrade cost).
- Vehicle yard BUILD = **ENGINEERING** panel (4:40-4:56), two pages:
  page 1 three icons (top not hovered: by elimination the cargo bee),
  construction bee (Dilithium 1G, Polycomp 1T, Trans Al 1T), work bee
  (moves resources in and out of buildings; Trans Al 1T); page 2 ground
  scout (unarmed scanning scout craft; Dilithium 3G by the stock, Polycomp
  1T, Trans Al 1T) and armed transport (armed personnel carrier;
  Dilithium 4G, Polycomp 3T, Trans Al 1T, Duranium 30KG). Clicking an
  icon flashes it magenta and adds "n ORDERED" to its tooltip; no queue
  line was shown.

## 5. Placement and construction look

- The hologram is a translucent copy of the building, drawn a little
  above the pointer. At 3:30 it was **red on concrete** in the west, beside
  the storage block and a parked unit (and the pointer turned into a red
  no-entry sign); at 3:31 **green** on free concrete north of the hub (the
  pointer a green arrow). Bare ground and deposits not tried.
- Construction (yard, 3:39-4:18): small black bees fly in (2-4 seen at
  once; the hub has 4 construction bees) and hop round the site; the
  building appears as flat dark-grey pieces that grow in place (pad and
  walls, like an untextured model), then pieces turn textured one by one
  until the whole is done. The hub upgrade went the same way: three dark
  lobes round the hub's base, one after another, then each textured.

## 6. Power

Hub readings (NEW WORLDS, power row):

| Time | Consumption | Demand met | Reserve |
|---|---|---|---|
| 2:55 | 87 % | 100 % | 17 % |
| 3:30 | 85 % | 100 % | 25-37 % |
| 3:35 | 85 % | 100 % | 60 % |
| 3:45 | 85 % | 100 % | 70 % |
| 4:00 | 87 % | 100 % | 87 % |
| 4:15 | 85 % | 100 % | 100 % |
| 4:25 | 88 % | 100 % | 98 % |
| 5:18 onward | 88 % | 100 % | 100 % |

Generator LOW throughout. The reserve filled about 1 % a second (17 % to
100 % in ~80 s), regardless of the yard being built; consumption went up
85 to 88 % once the yard was done. The hub upgrade changed nothing in the
power row (88 % before, during and after).

## 7. Controls (against play-tutorials.md)

All of play-tutorials.md's keys are confirmed. Corrections and additions:
- Message 26 (9:3 tilt) does have its clip: **Tutor26** (10:00.4, 4 s).
  The 7/1 message (25) plays at the end of **Tutor25** (18 s), which covers
  messages 24 and 25. play-tutorials.md's clip numbers for 25/26 are one off.
- **F1** first-person view: the COMMUNICATIONS box **jumps to the top
  centre** of the screen by itself as the view changes (9:20.0) and goes
  back to the bottom when F2 restores the sky view (10:59). It is not
  dragged.
- The locked view (double click on a building) was tried on the dome
  (10:50-10:56): the camera stays close on the dome.
- The playbook pop-up (12:47-12:57) shows as a column of square icons left
  of the Tricorder, with a long green bar (the tooltip) to their left.
- The Tricorder header reads TERRAIN AT <470:025> whenever the pointer is
  off the map (2:26-15:20; digits small), IMPASSABLE at 2:25.

## 8. Map and positions

- The full Tricorder (6:27-6:58, 15:25-15:33) shows the whole map as a
  tilted square; the colony sits near its middle on a mass of crosshatch
  (yellow west, green south, cyan north, magenta east, red south-east).
- Readouts near the hub, with the pointer just by the colony cross:
  **DILITHIUM AT <397:274>** (15:22), **<395:281>** (15:26, full
  Tricorder), **<402:272>** (15:37 onward, mini-map). So the C1 colony is
  round row ~395-402, column ~272-281, on or beside dilithium. This
  doesn't match play-tutorials.md's 373:264/275 for the hub area; perhaps
  their 373 is a misread 397, or a different spot. On a 0-511 scale it is
  the middle of a 64-block map, which suits a 64x64 map for C1.
- Terrain: temperate (grass, pines, pink-grey rock mesas, a lake south-west
  of the hub where the fight took place at 15:55-16:35).

## 9. Units and combat

- The group: a drag box at 11:08 picked up 6 vehicles (5 armed transports
  and 1 ground scout, MULTISELECTION; one more transport than the hub's
  TRANSPORT 4, so the scout ordered at 4:51 may be that one). Selected
  armed transport: FEDERATION, READY, SHIELD COVER HIGH.
- Sent to the mountain top at 12:21-12:45 (the camera stayed still, units
  crawled across view), then to the hub area at 13:29.
- Enemy scout: reported at 13:29, killed 15:41 (a flash by the vehicle
  yard), at once message 42 (attack from the south).
- The attackers: grey-white Romulan craft with a green glow (two seen
  together at 16:02), at the lake south-west of the colony; the transports
  fire yellow phaser bolts; a white ring explosion at 16:34; `fobjmet`
  16:34.6, message 45 at 16:38 names them Romulan combat units.
- A tall thin mirror-like sliver stands upright by the lake (13:49-16:30),
  most likely a terrain or rendering glitch in the VM.
- Unit speed: not measurable here (no Tricorder readings at both ends).

## 10. Messages (all Operations, the same portrait: young, light hair drawn back, red uniform with a white collar)

Counter n/N grows with each message (N = n), so the messages arrive one by
one. Time = first second the text shows.

| n | Time | Clip (start) | Paraphrase |
|---|---|---|---|
| 1 | 2:24 | Tutor08 2:24.5 | left-click buildings; their type and details show in NEW WORLDS top right |
| 2 | 2:32 | (Tutor08 cont.) | the panel's bottom buttons depend on the building; click till the hub is selected |
| 3 | 2:48 | Tutor09 2:48.6 | right-click drops the selection; select the hub again; more structures and vehicles come as the colony grows |
| 4 | 3:02 | Tutor10 3:02.2 | BUILD lists what can be built; click the construction yard and move the cursor north of the hub |
| 5 | 3:11 | (Tutor10 cont.) | move over free concrete until it turns green, left-click, building starts at once |
| 6 | 4:19 | Tutor11 4:19.3 | scroll south of the hub (cursor to the screen's bottom) and click the vehicle yard |
| 7 | 4:29 | Tutor12 4:29.8 | BUILD at the panel's foot, scroll the vehicles; only buildable ones show |
| 8 | 4:46 | Tutor13 4:46.6 | click the scout: it is made and drives to its rally point |
| 9 | 4:54 | (Tutor13 cont.) | the X bottom right closes ENGINEERING |
| 10 | 5:02 | Tutor14 5:02.8 | scroll to the hub, or press Home |
| 11 | 5:16 | Tutor15 5:16.4 | click the hub; a lit right arrow bottom left means an upgrade |
| 12 | 5:28 | Tutor16 5:28.6 | click the arrow to step through, click the improved hub icon: the upgrade starts |
| 13 | 6:20 | Tutor17 6:21.1 | Tab swaps mini-map and full-screen Tricorder |
| 14 | 6:30 | (Tutor17 cont.) | it is a topographic view of the world |
| 15 | 6:40 | Tutor18 6:40.4 | it colours minerals, terrain and each race's vehicles and buildings; the whole game can be played in it |
| 16 | 6:55 | Tutor19 6:55.7 | Tab again for the normal camera |
| 17 | 7:02 | Tutor20 7:02.5 | default view "bird's eye"; with NumLock numpad 4/6 rotate |
| 18 | 7:10 | (Tutor20 cont.) | numpad 8/2 zoom towards the cursor, "targeted zoom" |
| 19 | 8:03 | Tutor21 8:03.4 | arrow keys move forward, back, left, right |
| 20 | 8:23 | Tutor22 8:23.7 | F2: a much higher "sky view", same controls |
| 21 | 8:50 | Tutor23 8:50.3 | Ctrl + left click zooms in on the pointed spot |
| 22 | 8:58 | (Tutor23 cont.) | Ctrl + right click zooms out |
| 23 | 9:16 | Tutor24 9:16.8 | press F1: first-person view |
| 24 | 9:24 | Tutor25 9:24.9 | in this view: NumLock 8/2 targeted zoom, arrows the same, 4/6 rotate |
| 25 | 9:35 | (Tutor25 cont.) | numpad 7/1 raise and lower the camera |
| 26 | 10:00 | **Tutor26** 10:00.4 | 9/3 tilt the camera forward and back |
| 27 | 10:22 | Tutor27 10:22.5 | double-click a building: the camera jumps to a closer locked view |
| 28 | 10:27 | (Tutor27 cont.) | 4/6 orbit the locked building; right-click unlocks |
| 29 | 10:54 | Tutor28 10:54.9 | F2 back to the default/sky view; Shift+V to go to the vehicles |
| 30 | 11:07 | Tutor29 11:07.5 (weak, 0.46) | drag a box over the vehicles; they show in the details panel |
| 31 | 11:20 | Tutor30 11:21.0 | Ctrl+1 makes them a battle group (see the manual); Home to the hub |
| 32 | 11:41 | Tutor34 11:42.3 | right-click to deselect, click the hub: colony details now show |
| 33 | 11:57 | Tutor35 11:57.5 | 1 reselects the group, Shift+1 jumps to it |
| 34 | 12:19 | tutor36 12:20.4 | with them selected click on the mountain top: they go there defending themselves |
| 35 | 12:34 | tutor37 12:35.1 | double-click elsewhere on the mountain: the playbook pop-up; hovering an icon names it |
| 36 | 12:48 | tutor38 12:48.8 | scouts and science vehicles have their own playbook options, only when not grouped with other types |
| 37 | 12:55 | (tutor38 cont.) | they do engineering, science and medical scans; the science vehicle can carry officers |
| 38 | 13:07 | tutor39 13:08.0 | officers can be assigned to buildings that use their speciality (manual) |
| 39 | 13:16 | (tutor39 cont.) | Home, then 1 to make sure the group is selected |
| 40 | 13:29 | Tutor31 13:29.8 | move round the hub till the pointer turns green, click; wait for the vehicles there; an enemy scout is near |
| 41 | 13:39 | (Tutor31 cont.) | attack it with a playbook option; never let a scout scan the colony |
| 42 | 15:42 | Tutor32 15:43.5 | killing the scout set off an attack from the south; point at the attackers south of the hub and click near them |
| 43 | 15:50 | (Tutor32 cont.) | vehicles move to and attack enemies in range by themselves |
| 44 | 16:00 | Tutor33 16:00.6 | destroying the attackers ends the tutorial; good luck |
| 45 | 16:38 | **tutor40** 16:38.7 (0.48) | the attacking Romulan combat units are destroyed; training complete, well done |

Other sounds: `B_fcycom` 4:16.8 (yard complete, 0.85), `FED004b` 6:18.7
(0.99, as the hub upgrade finishes; F1 heard it as a scan started, so it
is probably a generic acknowledgement), `fobjmet` 16:34.6 (0.51),
`fmiswin` 16:48.7 (0.37, weak). tutor41 overlaps tutor40 at 16:38.7 but
scores lower (0.39); tut-2_05-07, 12, 17, 22, 27 not present (best 0.3-0.45
on noise at 3:5x).

## 11. Corrections to play-tutorials.md

- C1 is completed, not left unseen: message 45 (tutor40), debriefing box,
  MISSION REPORT with objective 16 only, then the main menu.
- C1 has 45 messages, not 44.
- Clips: message 24-25 are Tutor25, 26 is Tutor26 (not "none found").
- C1's starting stock is 660 / 160 / 2300 / 660 / 5600 / 6600 and size
  24/60, not the common 500/1000/... stock.
- C1's loading box lines differ from theirs (interface parameters, aural
  subroutines, surface data, geo-survey data), possibly a different stage
  of the same box.
- Hub area position: DILITHIUM AT <395-402 : 272-281>, not 373.

## Answers to vm-checklist items

- **Options and resolution**: Options has only Volume Control (three
  sliders) and Screen Size (640x480, 800x600, 1024x768) with Accept/Cancel;
  which size is set isn't visible. In play the fe5pt digits are ~4.5 px
  tall on a 640-wide scale (5 px glyphs), so the game runs at 640x480.
  Sure about the page; fairly sure about 640x480.
- **Panels never opened** (About, MISSION REPORT): About is a credits box;
  C1's MISSION REPORT: MISSION LOG plus the objective line. Sure.
- **Communications panel**: it moves to the top centre on its own in the
  F1 first-person view and back in the other views. Sure (one instance).
- **Build times**: construction yard ~44 s, hub upgrade ~40 s, generator
  LOW, demand met 100 %. Fairly sure (+-2 s).
- **Build costs**: hub upgrade Polycomp 4, Thermoconc 40, Duranium 1000
  (new); vehicle costs confirmed, scout Dilithium 3. Sure. Phaser tank,
  photon artillery, science block, phaser coil lab: not in C1.
- **Starting stock and size** (C1): 660/160/2300/660/5600/6600, 24/60.
  Fairly sure (polycomp digits small).
- **Colony capacity growth**: 60 throughout C1; the construction yard and
  hub upgrade did not raise it. Size grew 24 to 26 in ~7 min (one per
  ~3.5 min). Sure.
- **Power**: the hub alone, LOW: consumption 85-88 %, reserve filling ~1 %
  a second. No generators in C1.
- **Camera**: keys all confirmed; tilt is 9/3 with clip Tutor26. Heights
  not measured.
- **C1**: completed, with report; objectives 10-15 not shown, 16 is; tilt
  message clip Tutor26; hub digits: neither 272 nor 373 as asked, the
  readings near the hub are <397:274>, <395:281>, <402:272>. Sure on the
  end and the clip; positions fairly sure.
- **C1 briefing** green model: a long ground-vehicle hull with a raised
  cab, not a starship; which vehicle not sure.
- **Map** (C1 corners): not read; only the middle readings above.

## Placeholders (game.md)

- Build time: 40-44 s at LOW generator, demand met 100 % (matches the 40 s).
- Crew growth: +2 in ~412 s (2:58 to 9:50), so about one per 3.5 minutes
  in C1, much slower than the placeholder 30 s (C1's tutorial pace may
  differ).
- Power: hub consumption 85-88 % with hub, storage, vehicle yard, dome and
  construction yard; reserve +1 %/s.
- Capacity: no step seen (yard and hub upgrade gave none).
- Unit speed, mining, refining: nothing in C1.

## Still open

- Which screen size the Options page had selected.
- Where the ordered scout came out (hatch not watched long enough) and
  whether the 5th transport was there from the start.
- The dome building's name (never selected), the bare square patch.
- Enemy unit count and type name, weapon ranges.
- The map's corners and size; the meaning of TERRAIN AT <470:025> with the
  pointer off the map (last hover? digits uncertain).
- Whether the hologram was red because of the parked unit or the
  storage block next to it.
