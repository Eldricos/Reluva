# Star Trek: New Worlds: playing on the map

Reluva's first playable layer: the missions page's play mode (**F3**), with
the game's camera and controls and the colony's starting units to select
and move. It is built from what recordings of play show
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
| Tab | the full-screen Tricorder, units marked |
| I | the interface on/off |
| F3 or Escape | stops playing |

The numpad keys work with NumLock on or off. Not yet modelled: the
playbook (a double click with units selected), building selection and the
build menu, units defending themselves.

## What's modelled

- **Starting units:** four of the side's ground scouts (`ffs`, `gscout`,
  `rfgs`) in a row south of the hub. Every recorded colony starts with four
  vehicles of one kind selected (F1, R1); which kind isn't known.
- **Where units can go:** a tile is open unless its movement class, the
  tile flags' bits 16-17 ([gmf.md](gmf.md#tiles-and-ground-models)), is 0:
  holes, `water_01` and the rock formations, which nothing in the
  recordings crosses. Between tiles, the ground may not climb more than
  750 units in a quarter tile, so the walls between levels block and ramps
  don't. A unit finds its way round with A* over the 8192-unit tiles,
  straightening the path where a line is clear. Units sent to one spot
  spread over a small grid round it.
- **Selection:** the NEW WORLDS panel names one selected unit and its state
  (READY, EN ROUTE) as the game's does, or lists a MULTISELECTION; selected
  units carry the recordings' row of small orange marks above them.

## Placeholders

| What | Value | From |
|---|---|---|
| Ground scout speed | 4000 units a second (a block in 4 s) | by eye; to be timed |
| Units' drawn size | 4 × their models | the tutorial: a scout is a quarter of the vehicle yard's width beside it, against a sixteenth for the models |
| Bird's-eye view | 3.5 blocks away, 40° down | by eye |
| Sky view | 14 blocks, 65° | by eye |
| First-person view | 0.6 block, 6° | by eye |
| Spacing of a group's spots | 2500 units | Reluva's |

The code: `src/engines/newworlds/world.h` (units, passability, paths,
groups), the map view's game camera and picking (`viewers.h`,
`MapViewer::gameKey`), and the missions page's play mode.
