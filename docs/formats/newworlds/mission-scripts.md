# Star Trek: New Worlds: Reluva's mission scripts

The original keeps its missions' logic in `STNW.exe`; none of it is in the
data files. Reluva's scripts follow that logic as studied in the executable
([exe.md](exe.md#the-script-engine)): the same objectives and fail criteria,
triggers, conditions, delays and order of events, rewritten in Reluva's
own format (a handler per event, not the game's graph object by object)
and in its own words - every message paraphrases the game's, and a check
makes sure none shares a run of six words with the game's text. The first
scripts were written from recordings of play ([play.md](play.md)); the
recordings now serve to check them. The scripts are in
`src/engines/newworlds/mission_scripts.cpp`, read by `mission_script.h/.cpp`.

## What's in a script

- **Objectives** are the mission's own strings, named by their id in its
  table (`objective 16` is `fed01.stb`'s string 16), so their text comes
  from the player's game data and none of it is copied here. A `hidden`
  objective appears only when revealed (F1's temple capture); an
  `optional` one doesn't hold up the end.
- **Fail conditions** name the table's fail string: `lose 21 destroyed
  colony-hub`.
- **Messages are Reluva's own words** for what the game's messages say
  (the recordings' gist, see the play-*.md pages), with the sender the game
  used (a section, or "Incoming message", "Message from Starfleet", …).
- **Sites** are named places with map positions as the game gives them
  (eighths of a block, row first, [gmf.md](gmf.md#map-positions-in-the-game)),
  where a message named one.
- **Events** are what a game would report: a building finished, a set of
  buildings complete (`built-all vehicle-yard*4 power-generator*5|advanced-generator*3`),
  a unit nearing a site, a scan started, done, lost or of the wrong place,
  research done, a site entered or captured, something destroyed, a rival's
  hub destroyed or mission failed, a stock reached, a timer.
- **Generic rules** (`mission generic`) are the messages every mission
  shares: a building finished, a mine exhausted at a position, short of
  power, resources or crew, a rival's hub destroyed or mission failed.

The format is described at the top of `mission_script.h`. Its timers and
fail delays count the game's script seconds (16 ticks, 0.8 s;
[exe.md](exe.md#the-simulation-clock)), so the numbers are the game's own.

## How far they go

Scripted: all 51 playable missions and sides - F1-F14, K1-K14 and R1-R14
(R8 too, which has no recording), the joint missions J1-J4 and J6-J9 from
each of the three sides, the training missions C1 and C4-C8, and the bonus
missions C9-C11. Each plays through in a test: its handlers' events in the
order written, then time running, ending in a win.

Besides the player's own events, the scripts carry the game's attacks and
AI moves as `ai` actions (each side's scheduled assaults, the random raids
every few minutes, build orders and their cancelling), stance changes
(`hostile taubat`) and stock given or taken. Play carries out the `ai`
actions with the mission's own orders from the executable (1601 of the
scripts' 1610 order names are the game's; the rest are other kinds of
object: the bird-of-prey controllers, C1's artillery path controllers,
R9's trigger) and the stance changes
([game.md](game.md#the-other-sides-and-combat)), and feeds the scripts the
colony's buildings, its stock and building counts, key presses (the
training missions wait for keys), timers, what combat destroys, the
counts of every side's units and buildings, sightings of the sites, and
research (the game's topics, opened by the scripts' `research` and started
by the player), and scans and captures through the mission's own watches on them (the
executable's raids: [game.md](game.md#the-other-sides-and-combat)). The
capture events follow the raids: `entered` when the away team is in,
`captured` when it has held the building long enough.

The game's effect objects are written as actions and its area and
build-and-hold monitors as events ([exe.md](exe.md#script-effects)):
named rectangles (`zone`) watched with `inside` / `outside <who> <zone>`;
`spawn` (attack groups beamed in, the containment device, F8's gift of
tanks), `damage` and `beam-up` of a group, `move` along a route (Birds of
Prey, the tractor, J8's Metar starship), `meteors` and `storm` over a
zone (F2, R8; J2, K8, R1, R12), F2's `infect` / `cure` with its
`infection` and `cured` events, `top-up`, and `standing <what> <seconds>`
for the CFD missions' ten-minute holds. Counts of other sides' units and
groups work too (`count klingon-disruptor-tank == 14`).

Handlers can be switched: a label at the end of a handler's line (`on
scanned science temple as temple-scan dormant`), `arm` and `disarm` to turn
it on and off - the game's monitors that wake only after an earlier step,
or go to sleep. A fail can be delayed as the game's are (`fail 21
destroyed colony-hub`: the cause, the "you have failed" line, the end 20 s
later).

Each script ends with what the game does that the format can't say yet:
most often a fail announced and the mission ended 10-20 s later (Reluva
ends it at once or with timers), monitors the game switches on or off
part-way, attack groups spawned, damage, meteor and area storms, messages
to every side, and events about rival sides (a rival's scan, research or
pass).

## In Reluva

The missions page plays a mission's script a step at a time with **X**,
feeding it the walkthrough's events in order: the objectives (the game's
strings, ticked as they're met) and the latest messages are shown over the
map, and the title names the event. When the next event's handler is
still off and a timer may switch it on (a watch the game wakes some
seconds after a report), the steps let time pass first, one timer at a
time, until it's on; not past the mission failing. **Z** stops.
`reluva-tool script-check` walks each mission the same way, noting the
seconds of timers run before an event.
