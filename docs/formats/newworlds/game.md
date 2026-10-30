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
| Left click on a unit | selects it; its panel (name, side, READY or EN ROUTE, shield cover, hit points) in NEW WORLDS; above it up to five blinking marks in a zigzag, one for each fifth of its health |
| Left click on a building (the colony's, or another side's in sight) | selects it; its panel in NEW WORLDS |
| Double click on one of the colony's objects | a finished building plays its own sound (from where it stands); in F1 the camera follows the object (Up / Down nearer and further, numpad 4/6 orbit, 7/9 raise, 1/3 lower; F1, F2, F3 or Q let go) |
| Left drag | selects the side's units inside the box |
| Left click on the ground | sends the selected units there, firing on the way (cloaking units hold fire: the game's "move without attacking") |
| Left click on another side's unit or building (units selected) | sends them to attack it, following it (scouts and science vehicles scan it instead: a science scan) |
| Double click (units selected) on the ground, another side's object, or one of the colony's buildings | the **playbook**: a column of the game's order icons left of the Tricorder (the pointer over one names it); a click gives that order: on the ground attack en route, protect from location, move without attacking, return to the rally point; on another side's object follow and attack, assault, capture (armed APCs on a building), or the three scans (scouts and science vehicles alone); protect area (mobile shield generators on the colony's building). Any other click or Escape puts it away |
| A building's buttons (its panel) | STAFF (the hub: the officers' cards), BUILD (the hub and construction yard: the build menu; the vehicle yard: its vehicles), RESEARCH (science station, sickbay), - CREW / + CREW (the crew assigned to it), DEMOLISH (then YES or NO), INFO (the hub: the colony's details) |
| A JUMP TO icon (nothing selected) | the camera to the next of the colony's objects of that class; a group's icon selects it and goes to it |
| A click on an officer's card (STAFF), then on one of the colony's buildings or a vehicle that carries officers | moves the officer there |
| TRAINING (on the STAFF panel), then a course | starts the officer's training |
| Alt + A, then a click on the ground / on a target | attack en route (cloaking units fire too, decloaking) / assault: fire at it from where they stand |
| Alt + P, then a click on the ground / on one of the colony's buildings | protect from location: hold the spot, go after whoever attacks the colony, come back; with a building, mobile shield generators guard it ("protect area") and the rest hold beside it |
| Y | the selected armed vehicles drive back to the rally point |
| Alt + S, M, E, then a click on a target | a science, medical or engineering scan by the selected scouts and science vehicles |
| Alt + C, then a click on a building | the selected armed APCs capture it |
| V, then 1-9, 0 | the vehicles the yards can make, and the side's orders (in work, then waiting); a number orders one; a click on an order, then a second, cancels it (the cost back if a yard had started on it) |
| A, G, S, T, P | all the colony's APCs, ground scouts, science vehicles, tanks, photon artillery selected |
| Shift or Ctrl + B / V | the camera to the next of the colony's buildings / fighting vehicles |
| C | the colony's details (the hub's INFO page); a click on a construction in its queue, then YES, cancels it with the full cost back (at most 10 constructions ordered at once) |
| R, then 1-9 | the research topics open to the colony; a number starts one (several share the points) |
| O / Shift + O | the selected vehicles take aboard the officers of the building beside them / put them down |
| Left click on the TRICORDER | takes the camera there |
| TRICORDER's buttons | COMMS opens the communications panel; ALLY lists the other races and your posture towards each (a click, then FRIENDLY, NEUTRAL or HOSTILE; set neutral, a race turns hostile at its first shot at you); ZOOM steps the map X2, X1, X4 |
| Double click on the ground (nothing selected) | locks the camera on the spot; numpad 4/6 orbit it |
| Right click | drops the selection (or the camera's lock) |
| Double right click on the ground or the TRICORDER | moves the camera there |
| Ctrl + left / right click | zooms in on the pointed spot / out |
| Arrow keys | move the camera forward, back, left, right |
| Numpad 4 / 6 | turn |
| Numpad 8 / 2 | zoom in towards the pointer / out ("targeted zoom") |
| Numpad 7 / 1 | raise / lower the camera |
| Numpad 9 / 3, Page Up / Page Down | tilt |
| F1 / F2 / F3, Q | the game's cameras: low over the ground (2000 up, looking level; numpad 9 / 3 tilt it a little) / isometric (28000 up, 45 degrees) / close-up isometric (15000 up); Q the next |
| Z / X | keep the camera's place / go back to it |
| The pointer at the screen's edge (F2, F3) | scrolls that way, speeding up; in F1 the left and right edges turn the camera, the top and bottom raise and lower it |
| Space | the camera to the next of the colony's objects with an alert (lately hit, everyone beamed down, a mine exhausted); in F1 it follows it |
| Home | back to the colony hub |
| Ctrl + 1-5 | makes the selection a battle group (1-5 select it, Shift + 1-5 goes to it) |
| B | the build menu in the NEW WORLDS panel (B again closes it) |
| 1-9, 0 (menu open), or a click on its portrait | picks a building (its hologram follows the pointer) or orders a vehicle; < and > turn the menu's pages, X closes it |
| Left click (placing) | builds where the hologram is green |
| Right click or Escape (placing) | puts the hologram away |
| Tab | the full-screen Tricorder over the main view, units marked, a cross on the camera's target; the readout at the top names what's under the cross (an ore once its deposit is surveyed), the compass moving to the small Tricorder's line, and the small panel showing the main camera's view |
| I | the interface on/off |
| Escape | asks whether to leave the mission (Y / Return or YES leaves, N / Escape or NO stays; the mission runs on meanwhile); first it closes the menu or hologram |
| Pause | freezes the mission (as does the window losing the focus); again to go on |
| MULTISELECTION's entries | right click ticks one; a click selects it alone, a double click as on the object; DELETE drops the ticked from the selection, INVERT flips the ticks, the arrows turn the pages |
| G (missions page) | plays the mission through as the game runs it: a loading box over the three emblems, the briefing (Backspace skips it), a second loading box, play; once won or lost, the debriefing box, then the won or lost screen with the MISSION REPORT; CONTINUE (or Return) back to the page. Escape leaves at any point. F3 still starts play straight away |
| Ctrl+F5 | the Reluva menu: savestates of the mission being played, back to the launcher, quit (docs/savestates.md) |
| F5 / F6 (missions page) | plays the intro movie / the shown side's campaign movie (install.md#movies; the game's launcher has an intro button); Escape, Space, Return or a click skips a movie |
| K (missions page) | Cadet mode on or off for the missions played next: the colony's objects get twice their hit points instead of the race's bonus (the setup's Cadet box) |
| Ctrl + X (playing) | the script's walkthrough a step on (a testing aid) |
| COMMUNICATIONS' buttons | DELETE the message shown; the arrows to the first, previous, next and last; MISSION shows or hides the COMMAND panel (the mission and its objectives); X puts the panel away until the next message |

The numpad keys work with NumLock on or off. The keys
Alt + A, P, S, M, E and C (and Y) give the playbook's orders without it.

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
  ([colony.md](colony.md)) once what they need stands, as the game's
  does (play-tutorials.md, play-vm-c7.md): three large portraits a page,
  each with its key and name (bright when the stock can pay), in the
  training missions' order (storage, power, construction yard, vehicle
  yard, processor, mine, pad, science station, sickbay, security; the
  rest after, upgrades last); the one under the pointer, else the
  page's first, at the foot with a line of what it's for (Reluva's
  words) and its cost as the game writes it (dilithium in G, nitrium and
  duranium in KG, the rest in T); arrows, the page and X under it. A
  clicked portrait flashes magenta. The vehicle yard's (ENGINEERING) is
  the same, with the yards' work under it. While placing, the panel shows its cost,
  in red what the stock lacks.
- **Where buildings go:** on the tile grid, over as many tiles as the model
  needs, on free, flat ground near the colony's level (the ground round
  the hub), and not on a deposit
  block; a mining station's middle must be on one. The hologram is the
  model tinted green or red. Whether the game forbids building on
  deposits isn't known (Reluva's colony layout keeps them free for mines).
- **The start:** the colony and vehicles the mission itself places, as
  read from the executable ([exe.md](exe.md#starting-colonies)): vehicles
  placed on the hub start inside it. The mission's own starting stock.
- **Construction:** the cost comes off the stock at the order; if the
  stock can't pay, nothing is spent and the colony is told ("short
  resources"). The colony's construction bees build it
  ([exe.md](exe.md#bees)): they leave the hub and fly to the site's four
  corners (150 units a tick), up to four to a site, the sites ordered
  first served first; each bee at work puts in a quarter of the type's
  pace, as timed in the training missions with four bees
  ([play-vm-tutorials.md](play-vm-tutorials.md#build-times)), so one bee
  takes four times as long and a second site waits for bees. Power
  doesn't enter into it. A site done, its bees go on to one still short
  of them, or fly home. It is drawn as the recordings show it: an apron
  (the model pressed flat) for the first 15 % of the work, then the
  building rising out of it, the bees at work at its corners playing
  their `manipulate` animation (`fconbee`, `conbee`, `rcb`), and the
  others flying. When it's done, the script hears `built <type>`
  (Engineering's message, the game's spoken announcement; a hub the
  officer's report instead). The officers' other reports (delays, a
  vehicle finished, a yard suspended and back), the "under attack"
  voice, and the vehicles' answers to being selected, ordered or unable
  to go are heard as [exe.md](exe.md#sounds) gives them. Upgrades
  (improved and advanced hub and yards, vehicle yard stages) are in the
  menu once their building stands; they go on it at once, with no
  placing, and the bees build them where it stands. The other sides'
  colonies build at each type's pace, without bees.
- **Power:** the game's rules (exe.md#the-economy): generators and the hub
  supply their output a tick at 1.2 efficiency; each building keeps a
  power buffer it refills at up to its intake a tick and draws its use
  from; the supply, and the reserve behind it, is shared out over what's
  asked; DEMAND MET is the share granted. Below three quarters the colony
  is warned, once a minute. The title shows supply / asked.
- **Mining:** a mining station works the nearest ore within three blocks
  of its own, a tonne a take; of every eight takes two are the cell's own
  ore and one each kind, into its own store at the measured 2 t a second
  a mine. It goes on in cargo pods, as the game's ([exe.md](exe.md#bees)):
  once the mine holds a pod's worth (80 t the Federation's, 160 t the
  others') it packs its store evenly into store / size + 1 pods, and
  stops while four wait; its worker bee flies them out, one at a time, to
  places round it; each goes to the first processor that takes it
  (built, crewed, with its own worker bee, fewer than 17 coming); a
  building with more than one coming gets a cargo bee, which brings two
  a trip (the Klingons' and Romulans' one of their bigger pods) and
  empties them into its store. Every mine, processor and
  storage facility, and each yard, has a worker bee: those standing at
  the start get one free, a later one asks for one: a spare from the hub
  if there is one (the oldest asking first), else a vehicle yard makes it
  for that building, before the player's orders and at any colony size.
  Bees go home to the first hub made that isn't dying; a building lost
  takes those docked in it, kills those out near it (1.5 x its radius)
  and lets the rest go to another hub or a building asking (killed if
  none). The processor refines a quarter tonne a tick, in
  turn per ore, one for one: dilithium, talgonite, silicon, kelbonite,
  magnesite, dolamide into dilithium, polycomp, thermoconc, trans al,
  nitrium, duranium; every 32 ticks it packs its goods into pods once
  they make a pod's worth, and they go the same way to the first storage
  facility, into its own store. (The computer's colonies, without bees,
  send ore and goods straight on.) When nothing is left in reach the
  script hears `mine-exhausted` at the mine's block.
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
  its four pulses a tick apart and reloads after the last, artillery a
  shell (reaching 36850 at most). Every shot is a projectile
  ([exe.md](exe.md#vehicles-and-turrets)): a phaser pulse running out
  from its muzzle, a disruptor bolt at 2700 units a tick, a shell on its
  low arc (350 a tick, falling 3 a tick each tick), aimed where a moving
  target will be; it hurts the first object of another side its path
  crosses (no friendly fire), or nothing if it reaches its range or the
  ground, so shots can miss, and a nearer enemy in the way takes them. A
  defensive turret pays 300 of its power buffer a shot (a burst's four
  pulses and its reload, 1500): with its colony short of power it falls
  silent; vehicles and the natives' guns need none. A shot does its damage times the
  shooter's security skill + 1 (artillery's as it is). An armed unit or
  turret looks round every 8-11 ticks for the nearest enemy in range and
  in its arc (most turn all round; the Klingon armed APC's 90 degrees
  each way, the Romulans' 60, some natives' less),
  its turret meanwhile following the nearest in its reach; it fires only
  once its turret has turned onto the target, at the game's rate: a side
  fires at those it's hostile to, the player's units also at those
  hostile to the player, and anyone fires back at whoever hit it. The player's units driving
  somewhere don't stop to fight; sent to attack, they give chase. What
  runs out of hit points is destroyed: a blast, a building left as its
  flattened wreck (and gone from the economy), and the scripts hear
  `destroyed <name>`, the new counts (`count taubat-house 4`) and, for
  another colony's hub, `hub-destroyed <race>`. Shots are drawn as beams
  (orange phasers, green disruptors) or a flying shell; damaged or
  selected units show a hit point bar.
- **The AI** carries out the scripts' `ai` actions with the mission's own
  orders ([exe.md](exe.md#ai-colonies)), each once its mission time has
  come: an assault takes its count of the type from the side's idle units
  (or beams them in where the game's attack group does), asks the side's
  vehicle yards for any it lacks, and once it has them all drives along
  its waypoints, fighting what it meets, then goes for the nearest object
  of its target side, each unit first after whoever attacks it; a guard
  holds its spot (protecting its whole side, in mode 4); a patrol goes
  round and round, a sweep goes once and again when enemies are near its
  end, and in modes 5 and 6 both end as guards; `abort` stops a patrol or
  sweep, while guards and assaults take any message as their trigger. The orders the game runs from the start
  (the natives' patrols, the named builds) start with play, and those it
  runs for a human side too move the player's units (R8's
  prepositioners).
- **AI colonies** run an economy each, as the player's colony does
  (power, crew, officers, the yards' efficiency), but build and make
  units for nothing. A build order puts a site up on its spot once it's
  clear, drawn rising with the side's construction bee; a named build
  puts its building back whenever it's destroyed. Sites don't fight and
  aren't counted until they stand. A colony's AI (slots 0-2) goes only for other
  colonies, and fights natives only when they fire on it: F1's Klingons,
  hostile to every slot, pass the Taubat by as the recordings show. A
  script's stance action turns the side named towards the player.
- **The selection panel** as the executable draws it
  ([exe.md](exe.md#the-selection-panel)): name, side (the holder's when
  captured), state (OPERATIONAL, INCOMPLETE, CAPTURED, DESTROYED; a
  vehicle READY or EN ROUTE), SHIELD COVER (HIGH inside a shield field of
  its side, else BASIC) and a hit point bar. The colony's own buildings
  add their EFFICIENCY; the hub and generators the colony's power
  (GENERATOR LOW / MED. / HIGH by type, CONSUMPTION, DEMAND MET,
  RESERVE), consumers their own (SUPPLY, RESERVE); OFFICERS, CREW and
  POOL; the hub and construction yard the bees and transports; the
  vehicle yard what it's making (or SUSPENDED); a shield generator its
  COVERAGE; a storage facility what it holds itself, the construction
  yard the colony's stock; the resource processor its raw ores. The
  hub's overview isn't made yet.
- **The colony overview** ([exe.md](exe.md#the-colony-overview)): with
  nothing selected the NEW WORLDS panel shows the colony's size, its
  stock, the crew and pool, and JUMP TO: on the left the race's
  portraits of the hub, a mining station, the defensive weapon and
  artillery and the armed APC, each a click to send the camera to the
  next of that class (all fighting, science and scouting vehicles for
  the last); on the right battle groups 1-5, shown by their first member,
  a click to select the group and go to it. The hub's INFO switches its
  panel to the colony's details: its people (on the planet, in orbit,
  working, in the pools, dead) and each building and upgrade going up
  (BUILDING 1 OF 2 ..., ENGINEERING ...).
- **The sickbay and farms:** the sickbay's panel counts the crew HEALTHY,
  SICK (the disease's infected, less the dead), DEAD (those who died with
  their building) and their AVG. HEALTH; a farm's shows the colony's life
  support as a producer (CONSUMPTION, DEMAND MET, RESERVE).
- **Demolishing** ([exe.md](exe.md#demolishing)): DEMOLISH on a finished
  building (not the hub or a storage facility) asks yes or no; yes gives
  half its cost back at once, its officers to the hub and its crew
  walking back to the hub's pool (20 s), its worker and cargo bees off;
  once nobody is inside and none of its bees is docked there (the panel
  says EVACUATING meanwhile) it is destroyed the usual way: the blast,
  the wreck, `destroyed <type>` for the scripts.
- **Stores** ([exe.md](exe.md#stores)): the stock is what the storage
  facilities hold. The mission's stock is in the first, gifts and refunds
  go there, and the processor sends its refined goods there every 32
  ticks; costs come from the facilities in turn, a vehicle's only from the
  first one the colony had (even once it's gone, when it no longer counts
  in the stock). A storage facility destroyed takes its store with it, a
  processor its ore and goods.
- **A mission played through** (G): the loading box (the three emblems;
  the mission's label as the game gives it, FEDERATION LEVEL 01, JOINT
  LEVEL 03, TUTORIAL, BONUS ROMULAN MISSION; lines for the briefing,
  command routines and officers; the share done), the briefing, a
  loading box, play; won or lost, the debriefing box, then the race's
  emblem (won) or lose screen (`f_lose`, ...) with the MISSION REPORT
  under MISSION LOG: won, each objective COMPLETE or INCOMPLETE; lost, the
  fail criterion that fired, FAILED; and CONTINUE.
- **Cloaked units** shimmer while they fade (each corner of the model
  thrown about every frame: the player's own most in the middle of a
  fade, another side's most as it nears cloaked); the player's own,
  cloaked, are drawn as they are, another side's not at all.
- **The officers** (the hub's STAFF): a card each, with a portrait (the
  officer's own, as the mission gives it), rank (the mission's, in the race's titles) and name,
  section (Engineering, Security, Medical, Science, Operations, ...) and
  skill (BASIC 0.25, SKILLED 0.5, HIGHLY SKILLED 0.75, EXPERT 1), and
  where they are. TRAINING (lit while a course is left) lists the
  officers the mission lets train, each a click to start: the side's
  1000 training points a tick shared between the courses running (7.5 s
  alone), then the officer a band up; a card picked, the next of
  the colony's buildings or carrying vehicles clicked takes the officer
  (O and Shift+O still board and land a vehicle's). A building's crew
  can be set below its slots (- CREW): those over go back to the hub's
  pool; raised again, they walk over.
- **Orders** as the executable gives them ([exe.md](exe.md#orders)): a
  move fires on the way without stopping; attack en route does the same,
  cloaking units included; an assault fires from where the units stand,
  never closing in; protecting holds a spot, checks every 8 ticks for
  anything of the side with a live attacker (one that hit it and is still
  alive, uncloaked and hostile, however far off) and goes after the
  nearest, then the next, coming back once there are none (a mobile shield generator, which can't fire, drives
  its field up to the attacker of the building it guards). Units with
  nothing to do fire from where they stand. New armed vehicles drive to
  the side's **rally point**, a 4 x 4 grid of parking places in the
  hub's block, and so do those sent back with Y; the AI's vehicles no
  order asked for park at theirs.
- **Group counts, colony sizes and rivals** ([exe.md](exe.md#group-counts)):
  the scripts can count a side's small buildings, large buildings and
  armed vehicles (`count klingon-small-buildings <= 0`), hear any side's
  colony grow or shrink against its start (`colony-size taubat 50`;
  `colony-total` keeps counting its buildings others have captured, as
  J1's and R9's "down to half" monitors do), and
  the share of a colony the player's teams hold (`captured taubat-colony
  30`). The other sides' raid watches run as the game's do: theirs
  succeed only when the group is destroyed or their own units mark its
  block scanned (`captured-by`, `scanned-by <race> <what>`), since the
  computer never captures or scans; and a success sends Go_Futile to
  the raids it is wired to (J2's three hangar captures to each other, J3's
  per workshop; J4's and J6's to none), turning the player's watch
  `beaten` (under way) or `futile` (later).
- **Repairs, shields and cloaking** as the executable has them
  ([exe.md](exe.md#repairs)): vehicles mend a point at a time (faster
  with an engineer aboard); the colonies' buildings draw on a repair pool
  the construction yards fill. A hit inside a shield field of the
  target's own side is halved, and the target's security officer lessens
  it further. Cloaked tanks and APCs vanish 15 s after being seen,
  aren't shown or picked as targets while cloaked, drive on without
  fighting, and decloak to fire. A building still going up takes no harm
  and isn't fired at.
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
  joins the side's one list (no yard's own queue): a yard free, crewed
  and not razed takes the waiting order of the highest priority (a
  building's worker bee before the player's), oldest first, that its
  store can pay for, and pays as it starts; a yard lost loses its job,
  unpaid back, the order waiting again in its place for the next yard,
  which pays again; with no yard the orders wait; a yard puts up to
  300 power a tick from its buffer, times its efficiency (0.8 without an
  officer, 1 with one of skill 0.5), into the build value × 300, then the
  vehicle rolls out beside it (8 s) - bees into the hub. Officers posted
  in a building raise its efficiency.
- **Defences**: the shield generator, defensive weapon and defensive
  artillery, with the executable's costs, prerequisites and power
  (64000 buffer, 550 intake; 640 a tick for the generator, 160 for the
  turrets), so the vehicles that need them (the mobile shield generator,
  the Klingons' battery and cloaked tank, the Romulans' cloaked APC) are
  offered too. A shield generator's field shrinks with its efficiency:
  the 3 x 3 blocks round it above 0.6, a plus above 0.2, else its own
  block ([exe.md](exe.md#shields)).
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
  K7's pod watches read the same mark. The camera isn't kept to seen
  blocks (see [Left out for now](#left-out-for-now-to-restore-later)).
- **The Tricorder's ores**: blocks the side has surveyed (scouts, hubs,
  mines, science vehicles and most buildings survey round themselves)
  show their deposits in colour, by ore, on both Tricorders, and the
  readout names the ore under the camera ("DILITHIUM AT <row:col>"; else
  "TERRAIN AT"). Only what the side sees is marked on the Tab Tricorder.
- **Script effects** ([exe.md](exe.md#script-effects)): the scripts'
  spawns, group damage, beam-ups, routes, meteor storms (30 damage to
  what a meteor hits), lightning storms, the disease (each of the
  colony's plain crew, in its buildings and pools, visited in turn 16
  times a second: under 10 in 101 catches it, then under 4 costs 10 of
  their 256 health, and none left kills them; once the cure begins,
  under 60 gives 10 back, then cures them; its percentage is of everyone
  held when it began, and the cure is over once nobody counts as having
  it - one who died of it still does, as in the game),
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

## Left out for now, to restore later

Differences from the original kept on purpose while Reluva is being
built, to be put back at the end so play feels as the game's does:

- **The camera kept to seen blocks.** The game won't move the camera
  onto blocks the player's side can't see (exe.md#fog-of-war); Reluva
  lets it go anywhere, which helps when checking a map.

## Reluva's own additions, for later

Things the original doesn't have, to be added once play matches it, and
kept apart from the game's behaviour:

- **Saving and loading a mission** (done: Reluva's savestates,
  [savestates.md](../../savestates.md); Ctrl+F5 during play). The game has
  none: a mission is played through in one sitting. (The executable reads a
  `SAVEGAME` path from a developer settings file, `GAME.INI`, defaulting to
  `SAVE_FED.sav`, but nothing uses it; its `save.dat` is a debugging log of
  the simulation's cycles, written only.)

## Placeholders

| What | Value | From |
|---|---|---|
| Unit speeds | 1024 × the pace a tick: scouts 6140 units a second, tanks and armed APCs 3410, artillery and science vehicles 2050, unarmed APCs 1710 | the executable's played paces (exe.md#vehicles-and-turrets) |
| Animation speed | 10 frames a second | Reluva's (arf.md); the game plays its simulation's animations a frame a tick (20 a second) |
| Units' drawn size | 4 × their models | the tutorial: a scout is a quarter of the vehicle yard's width beside it, against a sixteenth for the models |
| F1's follow camera | 5 model radii behind (times distance / 800), 0.8 up, looking at the object (9 degrees down) | the executable (exe.md#cameras) |
| Escape's box | "LEAVE THIS MISSION?" | Reluva's wording |
| A transport's stop | twice the building's radius out on the side it comes from, then in through the dock and out to RELEASE at 100 a tick | the executable (exe.md#transports) |
| A pod aboard | hangs under its bee, 400 lower | the game hangs it at the bee's BAY_n_LOCK point |
| Space's alerts | objects hit in the last 41 ticks | the game's last 41 Tricorder redraws, and its reports too (beamed down, a mine exhausted) |
| Spacing of a group's spots | 1024 units | the game sends them all to the one point and they pack by their 1024 boxes |
| Build time | with four bees: processor 25 s, generator 30, construction yard 45, mine 60, vehicle yard 65, science station 75, security center 83 (timed); shield generator 27, photon turret 22, artillery 31; the Klingons' generator 23, yard 37, shield generator 22, turret 24; the Romulans' generator 28, yard 58, shield generator 48, turret 33, artillery 29; others 40 | the training missions, and the rest from each model's tiles against them (exe.md#bees) |
| Evacuating a razed building | 5 ticks at least, then until an unarmed APC has taken its crew aboard (they ride to the hub's pool); with no transports, they walk there in 20 s | the executable (exe.md#demolishing) |
| Goods from the processor to storage | sent straight to the first storage facility every 32 ticks | the game packs them into pods that bees carry |
| Costs, prerequisites | per side, upgrades included | the executable (exe.md#costs-and-prerequisites) |
| Power | buffers, intake and use a tick per building; hub 500, generators 500 (Romulan 1000) a tick at 1.2 efficiency; 640000 reserve; warned below 75 % | the executable (exe.md#the-economy) |
| Capacity | starts at 50, at most 90; accommodation +5, farms +10 | the executable |
| Mining | the 7 x 7 blocks round a mine, a tonne a take, 2 t a second carried by the bees | the executable; the carrying rate measured in C7 |
| Refining | a quarter tonne a tick in turn per ore, ore kind k into material k | the executable |
| Colony size | the colony's buildings and vehicles (no growth of its own) | the executable |
| Bees docked in a building destroyed | deleted, not killed: still counted in COLONY SIZE | the executable: the count drops only as an object dies, and the docked ones are deleted (one step inferred; not seen in a recording) |
| A vehicle half built in a yard destroyed | deleted with it, still counted in COLONY SIZE; one rolling out killed | Reluva's reading: the game makes it inside the yard as the job starts; its fate with the yard wasn't traced |
| A yard razed with a job in work | keeps it (no crew: paused) and isn't held up by it; the order waits again once the yard is gone | Reluva's: the game keeps the job at the yard until it dies; whether it holds up the evacuation wasn't traced |
| Yards taking orders | each tick a yard is free | the game offers the waiting orders as a yard frees, a new one stands or crew arrives |
| Warnings | once a minute while they hold | K2's power messages a minute apart; C8's exactly 60 s |
| Combat | hit points, damage, range, reload, magazine per type | the executable (exe.md#vehicles-and-turrets) |
| A phaser pulse's speed | 2500 units a tick | the game's is its beam model's length a tick (not read from the models yet) |
| An object's size to shots | its model's bounds radius (0.9 for vehicles) | the executable; the game grows it over attached parts (the bees' pods, which shots don't hit anyway) and a captured building's over the APC inside; without models (tests) 6000 and 1000 |
| A pulse's or bolt's height | flies level: it hits by its path on the map, and the ground never stops it | the game's flies in three dimensions and the ground (and its line checks) can stop it |
| A pulse's way | fixed when fired, from the turret as it moves | the game's turns with the turret |
| A vehicle facing a target outside its turret's arc | drives at it (for a point a target's radius short of it) and fires once it's in the arc | the executable (exe.md#orders) |
| Turrets' pitch limits | each gun's lift and dip (the phaser tank's 20 degrees up, 45 down) against the ground's heights; artillery not held | the executable (exe.md#vehicles-and-turrets) |
| Target finding | AI colonies go only for colonies | Reluva's (F1's recordings); the rest the executable's |
| An order's units | the side's idle ones, then its vehicle yards; none beamed in but by attack groups | the executable (exe.md#ai-colonies) |
| A unit in a building's way | its position inside the spot's box (half the footprint and 512 more) | the game keeps a box per unit as it moves; their size isn't known |
| Defences and shield generators going up (AI) | 40 s | Reluva's: the economy has no figures for them |
| Defences' build time | 40 s | Reluva's: not timed |
| The Klingon shield generator's model | `ksg` | the manual's picture (p. 87, a hooked arm) |
| An AI's mining station | stands where its order says, deposit or not | Reluva's: some orders' spots aren't on a deposit as Reluva reads the map |
| A repair pool at the start | empty | Reluva's |
| The Tricorder's relief | the five terrain layers as stacked sheets round the main camera's target, a layer a 40th of the shown width above the one below; Relief seen 45 degrees down, turning with the main camera; Starship 60 degrees down, north up; Relief orbit turning 3 degrees a second; Relief fixed north up; the zoom's tiles across the width at the middle; the full-screen one in the main view's place, left of the side panels | the executable gives the views, the layers and the per-size distances and heights (exe.md#the-tricorder), but not how its sheets sit in its camera's space, nor which way Relief fixed's quarter turn faces |
| The Tricorder's ground and ore colours | each point's terrain colour shifted for its block's class and the race; ores hatched in rings and blended at their edges by the blocks round each corner, unsurveyed blocks solid, the border dark | the executable (exe.md#the-tricorder) |
| Which playbook icon is which order | by the icons' pictures and names (`pbdefend` attack en route, `pbbombrd` assault, `pbprotct` protect from location, `pbprohre` protect area) | the executable loads them by number; the manual's pictures settle the two protects |
| Approach | within 1.5 blocks of a site | Reluva's |
| Crew walking to their building | 20 s | C8's yard crewed about 20 s after "not enough crew" |
| Rolling a vehicle out | a tank 9.5 s after the yard's work from the Federation's yard, 4 s the Klingons', 7.3 s the Romulans'; a scout two thirds of that | the yards' dock legs (exe.md#docking) |
| A lab's build time | 28 s | the training missions (21-35 s) |
| Scans, captures, officers | ranges, flags, strength, 80 %, 16.8 s, 16 seats, 1414 units | the executable (exe.md#scans-and-captures) |
| A scan no raid covers | its reply 2 ticks after coming in range, 8 s to finish | the game's routine reply (2 ticks; only F1, F2, J6, K1, K2, R1 and R2 have one); the finish is Reluva's |

The code: `src/engines/newworlds/world.h` (units, passability, paths,
groups, combat), `ai_sides.h` (the other sides and their orders),
`ai_data.h` (what the executable gives for every side), `field_work.h`
(scans, captures, officers), `research.h`, `script_effects.h`, `economy.h` (stock, construction, power, mining, crew),
`colony.h` (`ColonyLayout::spotAt`, where a building fits), the map view's game camera and picking (`viewers.h`,
`MapViewer::gameKey`), and the missions page's play mode.

## Headless runs

Play mode and the headless runs share one simulation (`MissionSim`,
`mission_sim.h`): the world, the economies, the AI, the fog, repairs,
field work, research, the script's effects, the mission script, and the
rules between them (officers' skills into efficiency, vehicles out of
the yards, razing, the scripts' sightings and monitors). The missions
page only draws it, passes on the player's orders and shows what the
script says, so what a headless run finds is what play does.

`reluva-tool play-check [-v] [-s F|K|R] <mission|all> <seconds> <file.cat>...`
plays missions on their real maps without a screen:
the player's colony as the mission places it, left to itself, the other
sides with their AI and economies, the fog, repairs, field work,
research, the script's effects and the mission script. It then prints
what looks wrong, as `stuck:` (a fault worth a look) or `waits:` (as
the game would):

- **stuck:** a unit placed on ground it can't move on; one that hasn't
  got a block nearer its destination in 30 s; a group idle on a leg for
  4 minutes; a build whose spot isn't clear after 2 minutes (naming what
  stands there); a unit that has fired at one target for 2 minutes
  without harming it.
- **waits:** a group still gathering after 3 minutes, for a vehicle yard
  its side doesn't have, its yards' queue, or yards standing idle (no
  crew once the hub is gone, too little power).

`-r` also loads a savestate halfway through into a second run and checks
both end the same (docs/savestates.md). `-v` adds the run's log: the script's AI orders, losses, won or lost.
`all` plays every map in the archives from each side it's played from,
and every mission with a script whose map is there under another name
(the tutorials and bonus missions, C1-C11, and F14, K14 and R14 on the
F12 map), as `levels.cpp`'s `missionMapNames` gives them. The keys a
tutorial asks for are pressed when it asks (as a trainee following it),
checked each second; anything else the script still waits for from the
player (buildings, scans, counts) is listed at the end as `script waits
for:`. 15 minutes of every mission take about two minutes.

The tutorials' first runs found C1 won in 21 seconds: its Klingon and
Romulan tanks were placed where the data lists them, on one spot, and
destroyed each other along with the artillery the tutorial waits for.
The executable lines each side's vehicles up in its own hub's block
instead ([exe.md](exe.md#starting-colonies)), and so does Reluva now.

What the first runs found (fixed): vehicles the mission places on the
hub's spot (R1's four tanks) were all kept inside it like bees, leaving
the colony undefended (only bees go in now; the rest park round the
hub); AI build orders waited forever for a spot their own starting
building already held; AI mines' spots aren't all on a deposit. What they still show, all from the data or
placeholders:

- F3's Klingon construction yard is ordered a quarter block from their
  hub, and J2's Klingon defensive artillery onto their construction
  yard's spot; neither is placed (the game's placement test may refuse
  them too).
- In J6, K8 and R9 the Metar fire at hubs whose repairs outpace them.
  Their pulses do 6 (a third of the type's figure, as the executable
  divides it), which shields and security officers cut to 1-2.
- A colony left to itself loses F1, F2, F3, R1, R2 and several joint
  missions within 15 minutes; in F1 to the first assault: a Klingon
  disruptor tank's 40-point shot kills a phaser tank (36 hit points)
  outright, against the phaser's four 6-point pulses, and the attackers
  drive in already facing their targets. Left to itself the colony's
  tanks don't fire until hit (the Federation is neutral to the Klingons
  in F1); the game leaves that to the player.
- Groups in the joint missions wait minutes for their yards: the random
  assaults order tanks faster than one yard makes them (about one every
  48 s).
