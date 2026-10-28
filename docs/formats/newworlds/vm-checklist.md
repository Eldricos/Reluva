# Star Trek: New Worlds: what to collect in the game

A checklist for playing the original game in the VM, cut down to what the
executable and the game's data can't answer: what only shows on the screen
(which message number is which, who destroys what, a few models) and a few
figures the code gives only roughly. Everything else the earlier lists asked
for is now read from the executable and the mission data
([exe.md](exe.md), [game.md](game.md)). Highest value first.

**Have running:** screen recording with sound, and the Tricorder panel
visible so its position readout is in every frame. Start each mission from
the campaign screen.

**Conventions:** map positions `<a:b>` are eighths of a block, row first
(0-511 on a 64-block map; a block's middle is 8n + 4), as the Tricorder
shows them ([gmf.md](gmf.md#map-positions-in-the-game)). Note the recording
time with every reading. Open COMMUNICATIONS whenever its counter changes and
never delete a message unread. **[shot]** = a screenshot is enough;
**[rec]** = needs a recording (timing, motion or sound).

## 1. Game-wide mechanics

- **Build times** [rec]: the executable gives how the bees build (tiles,
  four sections at once) but its time a tile doesn't match the Federation's
  timed builds. Time one Klingon or Romulan building (a power generator or
  vehicle yard) and one defence from the order to finished, with four
  construction bees; and the same building with one bee.
  ([exe.md](exe.md#bees))

## 2. Buildings and units (model matching)

Select each item and screenshot it close up, from two sides, with the
selection panel naming it ([colony.md](colony.md#buildings-and-models)).

- **Romulan hub upgrades** [shot]: the manual supports `rch_b` (a tower)
  as the first upgrade and `rch_c` (a base under it) the second; one
  close-up of an upgraded Romulan hub would confirm it. The manual's
  pictures settled the other model guesses (the storage facility, turrets,
  shield generators, transporter pad and utility hospital; colony.md).
- **Natives** [shot]: R2's black many-legged unit at 36:30 (an Orion tank
  `odv` or a Taubat tank `tgp`?), and J7R's magenta map marker.
  ([play-r2.md](play-r2.md), [play-j7r.md](play-j7r.md))
- **Klingon buildings** [shot]: an AI colony's mining station in a joint
  mission: does it stand on a deposit? ([play-k1.md](play-k1.md))

## 3. Interface

- **The star in star-system views** [shot]: the mission's planet is the
  body the camera centres on (the Federation Let's Play, every briefing);
  which body is the star isn't shown by shading (none glows). If a star
  can be told at all (a menu, a closer view), note it for J1, J2, J3, J7,
  J8, R2, R9, R12. ([play-fed-lp.md](play-fed-lp.md#star-system-views))

## 4. Per mission

Message numbers are the COMMUNICATIONS counter (`i/n`); the data gives each
message's text and trigger but not the order the counter shows.

**Federation**

- **F1** [rec]: which messages are 5-6, 14, 16 and 23-24 (the data narrows
  them to the processor line, the temple warning, the weak scans' lines and
  the Taubat-building line). ([play-f1.md](play-f1.md#still-to-look-at);
  another play's first ten, the Taubat's warning and the medical scan's
  lines among them, are in
  [play-fed-lp.md](play-fed-lp.md#messages-worth-noting): the numbers
  differ from play to play.)

**Klingon**

- **J7K** [shot]: the starting colony size (COLONY SIZE). ([play-j7r.md](play-j7r.md))

**Romulan**

- **R1** [rec]: the counter numbers of messages 5, 6 and 18 (another
  play shows the power warning repeating every minute, so they may be
  repeats; [play-14east.md](play-14east.md#r1)).
  ([play-r1.md](play-r1.md))
- **R2** [rec]: messages 3, 7, 8 and 23. ([play-r2.md](play-r2.md))
- **J1R** [rec]: the numbers of the two messages near 29:07 and 33:00 (the
  Federation's fail line and the temple scan's). ([play-j1r.md](play-j1r.md))
- **J2R** [rec]: who destroys the Klingon hub (the Federation's random
  raids or the Hubrin's?). ([play-j2r.md](play-j2r.md))
- **J6R** [rec]: messages 4 and 5. ([play-j6r.md](play-j6r.md))
- **J7R** [rec]: who destroys the Federation hub. ([play-j7r.md](play-j7r.md))
- **R12** [rec]: message 9 (64:45-66:20); who kills the Hubrin observer.
  ([play-r12.md](play-r12.md))
- **J8R** [rec]: the starting stock: J8 and F12 start with 160 more of
  dilithium, polycomp and transalumin and 1600 more of nitrium and
  duranium than the data's, on every side (J7 and the others exactly the
  data's): what adds it? Watch the stock from the first frame of play, and
  whether it changes at once. ([play-fed-lp.md](play-fed-lp.md#starting-colonies))
- **C11** [rec]: message 13 (16:46-18:30). ([play-c11.md](play-c11.md))

## 5. Missing data files

- **Nothing left here**: the game folder, the CD and its installer archive
  are all listed and checked against the install ([install.md](install.md)).

## 6. Left in Reluva (no VM needed)

Work on Reluva's side, listed so the whole picture is in one place.

- **Nothing left here.**

Done on Reluva's side since: Cadet mode (K on the missions page),
demolition by unarmed APC, messages' faces by their officer numbers, the
briefings' scenes (their units flying their paths), the Tricorder's colours
(the map's own, shifted as the game does), the guns' pitch limits, and from the
manual's check: Klingon and Romulan cargo bees carrying one pod,
cancelling constructions (and the cap of 10), F1's edge turning, Page Up
/ Down, the double right click, and the health marks over selected units. The
game's Bink movies now play too (the logos and intro at launch, a race's
movie when J9 is won), decoded by Reluva itself; and saving and loading (Reluva's savestates,
Ctrl+F5, for every engine).

Settled from the executable and the data since the last list: the camera
(it isn't held to seen blocks), speeds, docks, bees, roll-out, demolition,
placement, turning to fire, groups, transports, scans, officers, repairs,
animation speed, the script clock, power, shield cover, difficulty and
Cadet mode, the mission report, COMMAND, message faces, Tricorder colours,
bonus labels, every briefing (R8's too) and most missions' questions. A
second channel's recording ([play-14east.md](play-14east.md)) settled the
compass (the starting camera looks north) and K1's unseen messages (each
arrived with the one after it, as the script has them); a Federation Let's
Play ([play-fed-lp.md](play-fed-lp.md)) settled the Tricorder's ore rule
(the Tab Tricorder alone names ores, under its centre cross, once
revealed), J2F's mine (one the starting colony is placed with), J7's crystal units and the
fliers (Romulan disruptor tanks, Metar craft) and C9's map (`demo1`).
