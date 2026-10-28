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

- **Build times** [rec]: a site takes as long as its model's largest
  section (four bees at most; one bee does all four in turn), about 0.7 s
  a tile by the timed builds, the executable not giving the figure. Still
  wanted to check it: a vehicle yard timed cleanly, a defence, and any
  building with one bee. ([exe.md](exe.md#bees))

## 2. Buildings and units (model matching)

Select each item and screenshot it close up, from two sides, with the
selection panel naming it ([colony.md](colony.md#buildings-and-models)).

- **Natives** [shot]: R2's black many-legged unit at 36:30 (an Orion tank
  `odv` or a Taubat tank `tgp`? In another R2 play every enemy unit was
  the Orions'). ([play-r2.md](play-r2.md))

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

**Romulan**

- **R1** [rec]: message 18 (another play's first twelve are in
  [play-rom-lp.md](play-rom-lp.md#messages-worth-noting); 18 there is most
  likely a building's "complete" line). ([play-r1.md](play-r1.md))

## 5. Missing data files

- **Nothing left here**: the game folder, the CD and its installer archive
  are all listed and checked against the install ([install.md](install.md)).

## 6. Left in Reluva (no VM needed)

Work on Reluva's side, listed so the whole picture is in one place.

- **The natives' production speed**: with the Metar portals making a unit
  every 0.8 s (their build value of 16 ticks), J8's Metar guards destroy
  the Federation's AI colony about two minutes into an unplayed J8R.
  Check a portal's roll-out time in the executable, or a recording of J8
  where the Federation is the computer's.

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
fliers (Romulan disruptor tanks, Metar craft) and C9's map (`demo1`); the executable then explained J8's and F12's extra
stock (a starting vehicle yard's top-up). A Klingon Let's Play
([play-kli-lp.md](play-kli-lp.md)) then gave five Klingon build times, the
Hubrin as J2's attacker of the Klingon hub, J7's magenta marker (the
Hubrin guide), J7K's starting colony, C10's map (`coverd1`) and every
Klingon start's stock (the top-up wherever a yard is placed); and the
starts' colony sizes, the executable's count of objects and housing
([exe.md](exe.md#the-economy)). A Romulan Let's Play
([play-rom-lp.md](play-rom-lp.md)) recorded R8 for the first time,
settled the Romulan hub's two stages (the column, then the bladed base),
gave three Romulan build times and C11's time limit, and numbered R2's,
J1R's, J6R's and C11's open messages. The executable then explained the
rest: C11's deadline (48 minutes; the recording was cut), J7's
Federation hub (the other computer sides' random raids), R12's observer
(its patrol takes it into the natives' raids), the Klingon AI's mines
(J4, J6 and J7 only, none on a deposit), and that nothing marks a
star-system view's star (a plain model, no light).
