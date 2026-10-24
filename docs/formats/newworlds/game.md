# Star Trek: New Worlds: playing on the map

Reluva's first playable layer: the missions page's play mode (**F3**), with
the game's camera and controls, the colony's starting units to select and
move, buildings to place and build, the colony's economy, the other
sides' colonies and natives with their AI, combat, and the mission's
script running on what happens. It is built from what recordings of play show
([play-tutorials.md](play-tutorials.md) for the controls,
[play.md](play.md) for the rest). Figures the recordings can't give are
named placeholders, listed below, to be replaced with readings from the
running game ([vm-checklist.md](vm-checklist.md)).

## Controls

As the first tutorial teaches them:

| Input | What it does |
|---|---|
| Left click on a unit | selects it |
| Left drag | selects the side's units inside the box |
| Left click on the ground | sends the selected units there |
| Left click on another side's unit or building (units selected) | sends them to attack it (scouts and science vehicles scan it instead: a science scan) |
| S, M, E, then a click on a target | a science, medical or engineering scan by the selected scouts and science vehicles |
| C, then a click on a building | the selected armed APCs capture it |
| U, then 1-9, 0 | the vehicles the yards can make, and their work; a number orders one |
| R, then 1-9 | the research topics open to the colony; a number starts one (several share the points) |
| O / Shift + O | the selected vehicles take aboard the officers of the building beside them / put them down |
| Left click on the TRICORDER | takes the camera there |
| Double click on the ground (nothing selected) | locks the camera on the spot; numpad 4/6 orbit it |
| Right click | drops the selection (or the camera's lock) |
| Ctrl + left / right click | zooms in on the pointed spot / out |
| Arrow keys | move the camera forward, back, left, right |
| Numpad 4 / 6 | turn |
| Numpad 8 / 2 | zoom in towards the pointer / out ("targeted zoom") |
| Numpad 7 / 1 | raise / lower the camera |
| Numpad 9 / 3 | tilt |
| F2 | the sky view, much higher (F2 again: the bird's-eye view) |
| F1 | the first-person view, low over the ground (F1 again: back) |
| Home | back to the colony hub |
| Shift + V | to the selected vehicles (or the first one) |
| Ctrl + 1-9 | makes the selection a battle group |
| 1-9 / Shift + 1-9 | selects the group / centres on it |
| B | the build menu in the NEW WORLDS panel (B again closes it) |
| 1-9, 0 (menu open) | picks a building; its hologram follows the pointer |
| Left click (placing) | builds where the hologram is green |
| Right click or Escape (placing) | puts the hologram away |
| Tab | the full-screen Tricorder, units marked |
| I | the interface on/off |
| F3 or Escape | stops playing (Escape first closes the menu or hologram) |

The numpad keys work with NumLock on or off. The game's build menu is a
row of buttons (play-tutorials.md); here it is a numbered list. Not yet
modelled: the playbook (a double click with units selected), selecting
buildings, units building (here a building goes up by itself once
placed); the game's playbook and officer cards are keys here.

## What's modelled

- **Which way models face:** −y is a model's front ([mdl.md](mdl.md#moving-parts)),
  so a unit is drawn turned to put −y along its heading.
- **Starting units:** four of the side's ground scouts (`ffs`, `gscout`,
  `rfgs`) in a row south of the hub. Every recorded colony starts with four
  vehicles of one kind selected (F1, R1); which kind isn't known.
- **Where units can go:** a tile is open unless its movement class, the
  tile flags' bits 16-17 ([gmf.md](gmf.md#tiles-and-ground-models)), is 0:
  holes, `water_01` and the rock formations, which nothing in the
  recordings crosses. Between tiles, the ground may not climb more than
  1800 units in a quarter tile, so the walls between levels (a level is about
  1840) block and ramps (F1's climb up to 1800) don't; units spawned in a pit
  (F1's frozen pools) are moved to the nearest tile they can drive out of. A unit finds its way round with A* over the 8192-unit tiles,
  straightening the path where a line is clear. Units sent to one spot
  spread over a small grid round it.
- **Selection:** the NEW WORLDS panel names one selected unit and its state
  (READY, EN ROUTE) as the game's does, or lists a MULTISELECTION; selected
  units carry the recordings' row of small orange marks above them.

## The colony

- **The start:** the hub alone where the recordings put it
  ([colony.md](colony.md)), the stock every recording starts with
  (Romulans a little less; [play.md](play.md)), a crew of 20 and a
  capacity of 50.
- **The build menu** offers the side's buildings that have a model
  ([colony.md](colony.md)); one is dim until what it needs stands (a
  construction yard first, a resource processor before mines), bright
  when the stock can pay for it. While placing, the panel shows its cost,
  in red what the stock lacks.
- **Where buildings go:** on the tile grid, over as many tiles as the model
  needs, on free, flat ground near the hub's height, and not on a deposit
  block; a mining station's middle must be on one. The hologram is the
  model tinted green or red. Whether the game forbids building on
  deposits isn't known (Reluva's colony layout keeps them free for mines).
- **The start:** the colony and vehicles the mission itself places, as
  read from the executable ([exe.md](exe.md#starting-colonies)): vehicles
  placed on the hub start inside it. The mission's own starting stock.
- **Construction:** the cost comes off the stock at the order; if the
  stock can't pay, nothing is spent and the colony is told ("short
  resources"). Each type takes its own time, as timed in the training
  missions ([play-vm-tutorials.md](play-vm-tutorials.md#build-times)): the
  game's bees build at a pace the executable doesn't give as a number;
  power doesn't enter into it. It is drawn as the recordings show it: an
  apron (the model pressed flat) for the first 15 % of the time, then the
  building rising out of it, with two of the side's construction bees
  (`fconbee`, `conbee`, `rcb`) at its corners playing their `manipulate`
  animation (the game's sites have room for four). When it's done, the
  script hears `built <type>` (Engineering's message, the game's spoken
  announcement). Upgrades (improved and advanced hub and yards, vehicle
  yard stages) are in the menu once their building stands; they go on it
  at once, with no placing.
- **Power:** the game's rules (exe.md#the-economy): generators and the hub
  supply their output a tick at 1.2 efficiency; each building keeps a
  power buffer it refills at up to its intake a tick and draws its use
  from; the supply, and the reserve behind it, is shared out over what's
  asked; DEMAND MET is the share granted. Below three quarters the colony
  is warned, once a minute. The title shows supply / asked.
- **Mining:** a mining station works the nearest ore within three blocks
  of its own, a tonne a take; of every eight takes two are the cell's own
  ore and one each kind. The bees' carrying (about 2 t a second a mine,
  measured) feeds the processor, which refines a quarter tonne a tick, in
  turn per ore, one for one: dilithium, talgonite, silicon, kelbonite,
  magnesite, dolamide into dilithium, polycomp, thermoconc, trans al,
  nitrium, duranium. When nothing is left in reach the script hears
  `mine-exhausted` at the mine's block.
- **Colony size:** COLONY SIZE counts the colony's buildings and vehicles
  (those inside the hub too); the capacity starts at 50, accommodation
  adds 5 and a farm 10, up to 90.
- **The script:** the mission's script ([mission-scripts.md](mission-scripts.md))
  starts with play: `start` fires, its timers run, and the economy's events
  reach it; its messages show in COMMUNICATIONS and its objectives in
  COMMAND.

## The other sides and combat

- **The sides:** every slot of the mission but the player's is placed as
  the executable places it ([exe.md](exe.md#sides-and-combat)): the other
  colonies (F1's Klingons), the natives (Taubat, Metar, Hubrin, Orion),
  rivals; their buildings drawn with the map's, their units round the
  spots they're placed on (the bees left out). Each side's stances are the
  mission's; the TRICORDER marks the player's units green, those hostile
  to the player red, the rest yellow.
- **Combat** runs at the game's 20 ticks a second with the executable's
  figures: hit points (the player's own times 1.2 for the Federation, 1.3
  for the Romulans), weapon damage, range and reload; a burst weapon fires
  its four shots a few ticks apart, artillery a shell. An armed unit or
  turret finds the nearest enemy in range: a side fires at those it's
  hostile to, the player's units also at those hostile to the player, and
  anyone fires back at whoever hit it. The player's units driving
  somewhere don't stop to fight; sent to attack, they give chase. What
  runs out of hit points is destroyed: a blast, a building left as its
  flattened wreck (and gone from the economy), and the scripts hear
  `destroyed <name>`, the new counts (`count taubat-house 4`) and, for
  another colony's hub, `hub-destroyed <race>`. Shots are drawn as beams
  (orange phasers, green disruptors) or a flying shell; damaged or
  selected units show a hit point bar.
- **The AI** carries out the scripts' `ai` actions with the mission's own
  orders: an assault takes its count of the type from the side's idle
  units (or beams them in where the game's attack group does) and drives
  along its waypoints, fighting what it meets, then goes for the nearest
  enemy; a guard holds its spot and goes for enemies near it; a patrol
  goes round; a build puts the building up after its delay; `abort` stops
  an order. The orders the game runs from the start (the natives'
  patrols) start with play. A colony's AI (slots 0-2) goes only for other
  colonies, and fights natives only when they fire on it: F1's Klingons,
  hostile to every slot, pass the Taubat by as the recordings show. A
  script's stance action turns the side named towards the player.
- **Scans** as the executable has them ([exe.md](exe.md#scans-and-captures)):
  a scout or science vehicle drives into sensor range (60000, 18000, plus
  the target's size) and scans for as long as it stays; the mission's
  raids turn a scan kept up for their times into `scan-started`, then
  `scanned <kind> <target>` (or `scan-lost`), the kind `strong-…` when an
  officer aboard is good enough (skill 0.5 in the scan's speciality). The
  target is named by the mission's label for its group (`taubat-mine`),
  its type and the script's sites within 2 blocks. A scan no handler
  waits for gets `scanned-nothing <kind>` (the game's stock reply); one a
  handler waits for but no raid covers takes 1 s to start and 10 s more.
  A pale blue dotted line marks a scan going on.
- **Captures:** an armed APC fires on the building to 80 %, drives up and
  sends its team in; 16.8 s later the building is held (CAPTURED over
  it, its hit points full) while the APC keeps the order. The scripts
  hear `entered`, then `captured` once a raid's hold time is up (at once
  where none watches it), `held <target> <seconds>` each second,
  `capture-lost` or `vacated` when the team comes out.
- **Vehicles** as the executable makes them ([exe.md](exe.md#vehicle-production)):
  U lists the side's vehicles whose yard, upgrades and labs stand (the
  labs are in the build menu once the science station is up); an order
  takes the cost and queues at the least busy yard; a yard puts up to
  300 power a tick from its buffer, times its efficiency (0.8 without an
  officer, 1 with one of skill 0.5), into the build value × 300, then the
  vehicle rolls out beside it (8 s) - bees into the hub. Officers posted
  in a building raise its efficiency. Not yet buildable: defensive
  weapons and artillery, so the vehicles that need them aren't offered.
- **Crew and efficiency** as the executable has them
  ([exe.md](exe.md#crew-and-efficiency)): every building works at 0.4 ×
  (health + power fill + officer) × crew; generators and the hub give
  their output times it (a hub without a command officer 400 a tick).
  The side's 150 people (the officers among them) start in the hub's
  pool; each building's crew walk over from a pool when it's built
  (20 s), or are beamed down by a transporter pad, or wait; the vehicle
  yard needs its crew before it works. Everyone on the planet costs life
  support, which the hub pays for in power (about 180 a tick for 150
  people) and farms help with. The NEW WORLDS panel shows CREW and POOL.
- **Fog of war** as the executable has it ([exe.md](exe.md#fog-of-war)):
  each block holds what each side covers now, from its objects' fog
  classes, redone a sixty-fourth at a time; other sides' units and
  buildings show (on the map, on the Tricorder, to click on) only where
  the player's side sees. The scripts hear `seen <site>` when the
  player's side first sees a site's block, and `cell-scanned <site>` when
  its block is first scanned (a science vehicle or a building beside it);
  K7's pod watches read the same mark. Not modelled: the camera kept to
  seen blocks, shield fields, the mineral readout.
- **Script effects** ([exe.md](exe.md#script-effects)): the scripts'
  spawns, group damage, beam-ups, routes, meteor storms (30 damage to
  what a meteor hits), lightning storms, the disease (among the crew),
  stock given, taken and topped up are carried out on the map, and the
  scripts hear who comes into or leaves their zones, what has stood how
  long, beam-ups and the infection's spread. Meteors fall as streaks,
  storms flash, beams shimmer.
- **Research** as the executable has it ([exe.md](exe.md#research)): a
  script's `research <topic>` opens the mission's topic once it has been
  sent as many times as the game wants; R lists the open ones in the NEW
  WORLDS panel and a number starts one; the side's 1000 points a tick are
  shared between those running (F1's plague: 7.5 s alone). Finished, the
  scripts hear `researched <topic>`.
- **Officers** start where the mission posts them; the panel lists those
  aboard a selected vehicle.
- **Sightings:** the scripts hear `approach <who> <site>`
  when a unit is within 1.5 blocks (who: the unit's kind for the player's,
  its race for another side's), once each.

## Placeholders

| What | Value | From |
|---|---|---|
| Ground scout speed | 4000 units a second (a block in 4 s) | by eye; to be timed |
| Animation speed | 10 frames a second | Reluva's (arf.md) |
| Units' drawn size | 4 × their models | the tutorial: a scout is a quarter of the vehicle yard's width beside it, against a sixteenth for the models |
| Bird's-eye view | 3.5 blocks away, 40° down | by eye |
| Sky view | 14 blocks, 65° | by eye |
| First-person view | 0.6 block, 6° | by eye |
| Spacing of a group's spots | 2500 units | Reluva's |
| Build time | per type: processor 25 s, generator 30, construction yard 45, mine 60, vehicle yard 65, science station 75, security center 83; others 40 | the VM recordings of C1, C4-C8 (play-vm-tutorials.md) |
| Costs, prerequisites | per side, upgrades included | the executable (exe.md#costs-and-prerequisites) |
| Power | buffers, intake and use a tick per building; hub 500, generators 500 (Romulan 1000) a tick at 1.2 efficiency; 640000 reserve; warned below 75 % | the executable (exe.md#the-economy) |
| Capacity | starts at 50, at most 90; accommodation +5, farms +10 | the executable |
| Mining | the 7 x 7 blocks round a mine, a tonne a take, 2 t a second carried by the bees | the executable; the carrying rate measured in C7 |
| Refining | a quarter tonne a tick in turn per ore, ore kind k into material k | the executable |
| Colony size | the colony's buildings and vehicles (no growth of its own) | the executable |
| Warnings | once a minute while they hold | K2's power messages a minute apart; C8's exactly 60 s |
| Combat | hit points, damage, range, reload, magazine per type | the executable (exe.md#vehicles-and-turrets) |
| A burst's shots | 3 ticks apart | Reluva's |
| Reach on a building | 6000 units past its middle | Reluva's |
| Target finding | the nearest enemy in range, every tick; AI colonies go only for colonies | Reluva's (F1's recordings for the last) |
| An order's units | the side's own, any lacking made up at its first building | Reluva's: the AI's production isn't modelled |
| Guards | go for enemies within 2 blocks of their spot, back past 4 | Reluva's |
| Approach | within 1.5 blocks of a site | Reluva's |
| Crew walking to their building | 20 s | C8's yard crewed about 20 s after "not enough crew" |
| Rolling a vehicle out | 8 s after the yard's work | the recordings' scout (build value 0) |
| A lab's build time | 28 s | the training missions (21-35 s) |
| Scans, captures, officers | ranges, flags, strength, 80 %, 16.8 s, 16 seats, 1414 units | the executable (exe.md#scans-and-captures) |
| A target's size | buildings 6000, vehicles 1000 | Reluva's (the game uses the model's bounding radius) |
| A scan no raid covers | 1 s to start, 10 s more | Reluva's (the scripts' own targets) |

The code: `src/engines/newworlds/world.h` (units, passability, paths,
groups, combat), `ai_sides.h` (the other sides and their orders),
`ai_data.h` (what the executable gives for every side), `field_work.h`
(scans, captures, officers), `research.h`, `script_effects.h`, `economy.h` (stock, construction, power, mining, crew),
`colony.h` (`ColonyLayout::spotAt`, where a building fits), the map view's game camera and picking (`viewers.h`,
`MapViewer::gameKey`), and the missions page's play mode.
