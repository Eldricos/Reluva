# Star Trek: New Worlds: watched in play, mission F1

What the original game does, as seen in a 32-minute recording of
Federation mission F1 ("Power Corrupts") from start to finish:
<https://www.youtube.com/watch?v=W4TaXcESxiQ> (1280×720, 60 fps). This is
observed behaviour, the same as playing the game; times below are the
video's. October 2026.

Clips were placed in the recording by cross-correlating its audio with the
`Media/FedVO` files (scores 0.5-0.76 for the clean matches against 0.1 or
less anywhere else; 0.19 against 0.03 for `F1005`, which plays over loud
music). Pages were read from the screen.

## The briefing (0:00-4:00)

The briefing isn't a menu: it's a sequence drawn by the game engine with the
mission's own map and models, a page of the mission's string table
(`fed01.stb`) at a time, each page in a panel (heading in a green bar, text
in a small capitals-only font, probably `fe5pt`) that moves between the
screen's corners. A compass strip runs across the top during the map views
(`--210--+--220- S W -230-`, `[ N ]`), and two buttons sit at the bottom
right: **NEXT** and **MISSION**.

| Time | Shown | Page (`fed01.stb` id) | Speech |
|---|---|---|---|
| 0:05 | the galaxy, a red and blue nebula | STELLAR CARTOGRAPHY: Tabula Rasa Systems (2) | `F1005` (34.3 s, 0:05.15) |
| 0:40 | the star system, planets | PLANETARY DATA: Karazar System. Planet of Operations: Karazin (4) | (`F1006`, 19.9 s by size, would fill it exactly) |
| 1:00 | flying over the F1 map behind a shuttle: trees, cliffs, ice caves, the Taubat pyramids | no panel | (`F1007` + `F1008`, 38 s by size, against 37.75 s) |
| 1:38 | the colony site: a building on a concrete pad; a minimap inset | PRIMARY OBJECTIVE 1 (6) | (`F1009`, 14.7 s; next page 1:52.75) |
| 1:53 | the map as a blue relief grid, the Taubat area marked in magenta, yellow, red | PRIMARY OBJECTIVE 2 (7) | (`F1010`, 18.2 s; next page 2:11.25) |
| 2:11 | the Taubat temple as a dark wireframe model on a grid | PRIMARY OBJECTIVE 3 (8) | (`F1011`, 18.0 s; next page 2:29.5) |
| 2:30 | the relief grid again, a site marked | PRIMARY MISSION ADVISORY: Place a Mining Station here… (9) | `F1012` (6.2 s, 2:29.83: "This mineral field should supply complete resources…") |
| 2:38 | Taubat pyramids and flat triangular structures | GENERAL MISSION ADVISORY: Taubat will fight back if attacked (10) | `F1013` (10.7 s, 2:38.32) |
| 2:49 | the pyramids from the ground | PRIMARY MISSION RESTRICTIONS: There can be no Taubat fatalities (11) | (`F1014`, 12.2 s; next screen 3:01.75) |
| 3:02 | the Federation emblem, NEXT and MISSION | no panel | `F1015` (10.1 s, 3:02.08) |
| 4:04 | the game | | |

So, for F1 at least:

- **The voice clips are the briefing's narration, in number order**, one
  per step, and each step lasts as long as its clip: the four matched clips
  start within 0.3 s of their page appearing, and the unmatched ones' sizes
  (64 kbit/s) fit the gaps between the pages. The flyover carries two clips.
- **The pages go in the table's order**, skipping those not shown as
  panels: 3 (INCOMING TRANSMISSION), 5 (the communiqué) and the pass and
  fail pages. For the panels shown, the clip number is the page's id + 3
  (`F1005` with page 2, `F1012` with 9, `F1013` with 10), though `F1015`
  (with no page) breaks that.
- The emblem screen is the briefing's end; the player waits there (about
  50 s in the recording) and starts with **MISSION**.

## In play

- **Interface** (640×480 scaled up): a **NEW WORLDS** panel at top right
  (the selection: "MULTISELECTION" with an icon and health bar per unit, a
  unit's name, side and state, "KLINGON / READY / SHIELD COVER BASIC"; the
  colony; build menus), **COMMUNICATIONS** at the bottom (a portrait, the
  sender, the message, "1/1", DELETE, playback arrows, MISSION) and
  **TRICORDER** at bottom right (a minimap, "TERRAIN AT <407:209>" or
  "RELIEF" over it, three buttons). A compass strip at the top.
- **The start:** the camera over the colony hub (a hexagonal building) on a
  large concrete pad, four vehicles of one kind selected. 18 s in, the
  colony panel reads `COLONY SIZE = 21/50` and six materials: `DILITHIUM =
  500`, `POLYCOMP = 988`, `THERMOCONC = 2211`, `TRANSALUMIN = 495`,
  `NITRIUM = 3950`, `DURANIUM = 5000` (the start, or nearly), then `JUMP TO`
  and building icons.
- **First message:** "INTELLIGENCE: Command advises caution: a Klingon
  colony has been established to the south." A Klingon ground scout attacks
  within the first minute (4:50).
- **Tricorder coordinates** run to at least 407 and 209 on a 64 × 64 block
  map: presumably the 16 × 16 squares per block of the placed-object grid
  (1024 × 1024), see [gmf.md](gmf.md).
- **Buildings** stand on concrete, each with a yellow-striped apron (the
  `pad` texture's stripes, see [geo.md](geo.md)) in front of its door.
- **Objectives:** a COMMAND panel lists them; at the end a **MISSION
  REPORT** over the emblem: "MISSION LOG." then each in-game objective
  string (`fed01.stb` 16-19) as "MISSION OBJECTIVE: <string> - COMPLETE".

None of the interface's own text (the materials' names, `COLONY SIZE`,
`JUMP TO`, `MULTISELECTION`, the Intelligence message) is in any of the
archives' string tables or anywhere in their bytes (searched as ASCII and
UTF-16): it's presumably in `STNW.exe`.

## Through the mission (4:04-32:00)

Read off the Communications panel (sampled every second; its counter says
the mission sends 41 messages, of which the ones below were on screen),
the COMMAND panel and the colony panel.

### Starting stock

The colony panel at 4:17, before anything is spent: `COLONY SIZE = 20/50`,
`DILITHIUM = 500`, `POLYCOMP = 1000`, `THERMOCONC = 2300`, `TRANSALUMIN =
500`, `NITRIUM = 4000`, `DURANIUM = 5000`. Five seconds later construction
has started taking from them (988, 2211, 495, 3950). The second number of
the colony size, its capacity, grows as the colony is built: 50, 55 (by
10:26), 65 (17:04), 80 (22:20), 90 (22:40); the first (the crew?) from 20
to 40.

| Time | Size | Dilithium | Polycomp | Thermoconc | Transalumin | Nitrium | Duranium |
|---|---|---|---|---|---|---|---|
| 4:17 | 20/50 | 500 | 1000 | 2300 | 500 | 4000 | 5000 |
| 6:10 | 21/50 | 430 | 986 | 2141 | 495 | 2350 | 5000 |
| 10:26 | 28/55 | 392 | 920 | 1704 | 414 | 1430 | 3860 |
| 17:04 | 33/65 | 560 | 976 | 1362 | 489 | 467 | 3127 |
| 22:20 | 38/80 | 714 | 1119 | 1046 | 577 | 137 | 3147 |
| 22:40 | 40/90 | 709 | 1114 | 946 | 552 | 265 | 2445 |

Dilithium, Polycomp and Transalumin rise again after 10:26, when mining and
processing are running. A selected building's panel reads, for example,
"VEHICLE YARD / FEDERATION / OPERATIONAL / SHIELD COVER BASIC / EFFICIENCY
80%", then CONSUMER / SUPPLY / RESERVE figures.

### Objectives

The COMMAND panel lists the mission's title and stardate (`fed01.stb` 1 and
24), then each in-game objective as "MISSION OBJECTIVE: <string> -
INCOMPLETE" or "- COMPLETE": at 13:28, strings 16-18, all incomplete; at
21:04, 16 complete; by 29:27, 16-18 complete and **a fourth added**, 19
("Capture the Taubat temple."), incomplete. So objective 19 only appears
once the engineering scan is done. All four complete at 31:19 ("All
objectives complete. Well done."), and the mission report follows.

### Messages

Each has a sender (the section, with its officer's portrait), the text and
the counter. Long ones end in `<MORE>` and continue in the next message.

| Time | # | From | Message |
|---|---|---|---|
| 4:12 | 1 | Intelligence | Command advises caution: a Klingon colony has been established to the south. |
| 5:03 | 2 | Engineering | Construction yard complete |
| 5:24 | 3 | Intelligence | Sir, incoming vehicle detected. It's Klingon... and it's targeting the colony! |
| 5:49 | 4 | Engineering | Power generator complete, sir. |
| 7:22 | 7 | Engineering | Vehicle yard complete |
| 11:21 | 8 | Science | Scanners running now, sir. |
| 11:49 | 9 | Science | Scan complete, but the results are ambiguous - I'd like a second opinion. |
| 13:12 | 10 | Science (another officer) | Scanners running: using low-pass filters to improve the signal. |
| 13:20 | 11 | Science | Scan complete: this appears to be a crude mining facility, but radiation levels are unusually high. |
| 15:26 | 12 | Security | Reading disruptor discharges. It would seem the Klingons are attempting to take this world. |
| 17:41 | 13 | Operations | Construction delayed: insufficient resources. Build mines to collect raw materials and build a resource processor to refine the minerals into resources. |
| 18:13 | 15 | Operations | Vehicle manufacture delayed! Insufficient crew. Assign crew to this building. Build accommodations and a transporter pad to beam more crew down. |
| 20:54 | 17 | Engineering | Accommodation block complete, sir. |
| 21:00 | 18 | Command | Colony expansion complete. (objective 16) |
| 25:20 | 19 | Medical | We have found the plague epicenter. Suggest running a medical scan on the central structures. |
| 25:36 | 20 | Medical | Full diagnostics show nothing of interest here, sir. |
| 25:50 | 21 | Medical (another officer) | Running a full diagnostic scan now. |
| 25:52 | 22 | Medical | Scan lost: we'll need to try again if you want that diagnosis. |
| 26:02 | 25 | Medical | Diagnostic scan complete: we know so little about Taubat biology that it's hard to be sure, but this looks like radiation sickness to me. <MORE> |
| 26:00 | 26 | Medical | I'd like some research done on this back at the colony: the sickbay staff should be able to handle it. |
| 26:51 | 27 | Medical | Research confirms the doc's hunch: the plague is a form of radiation sickness, aggravated by mutated forms of common Taubat pathogens. <MORE> |
| 26:59 | 28 | Medical | Only an artifical radiation source could account for this level of contamination. We've got to find that source and shut it down while there's still time. |
| 27:10 | 29 | Command | Medical research complete. (objective 17) |
| 27:27 | 30 | Medical | The Taubat are dying: let's find the source of that radiation now. |
| 28:19 | 31 | Engineering | Scanners show a strong energy source west of the Taubat colony, position <332:152>. Suggest we investigate. |
| 28:41 | 32 | Engineering (another officer) | Running a full systems scan now. |
| 29:06 | 33 | Engineering | Systems scan complete: it's a pretty sophisticated reactor, but the Taubat haven't been maintaining it properly. <MORE> |
| 29:14 | 34 | Engineering | The primary containment field is out of alignment and leaking badly. It's a simple fix, but we'll need to get in there fast. |
| 29:34 | 35 | Command | Engineering scan of temple complete. (objective 18; objective 19 appears) |
| 29:38 | 36 | Security | We must be careful not to provoke the Taubat. A single APC should be used to take the temple. |
| 29:42 | 37 | Intelligence | Sir, the Taubat are demanding that we leave this area. We have 5 minutes to withdraw. |
| 30:14 | 38 | Engineering | Mining operation at 404, 220 reports mineral exhaustion. |
| 31:03 | 39 | Engineering | Away team inside, sir. We'll start on the field realignment immediately. |
| 31:11 | 40 | Engineering | The containment field is realigned, and radiation levels are falling off. Looks like we were just in time, sir. |
| 31:19 | 41 | Command | All objectives complete. Well done. |

(The player browses back and forth through the messages at 25:39-27:24, so
some counters there read like "19/20", "28/30".) Messages 5-6, 14, 16, 23
and 24 arrived while the panel wasn't showing.

So the mission's script reacts to: the start (1), buildings finishing
(generic "<building> complete" messages from Engineering; `fed01.stb`'s own
"Vehicle yard completed, Sir." isn't the one shown), an enemy unit
approaching (3), combat (12), shortages (13, 15), the scans (a science
vehicle with a doctor or engineer, each scan a "running" message and then
its result, a scan of the wrong place giving "nothing of interest", a scan
interrupted giving "scan lost"), research at the sickbay, and the APC's
away team entering the temple (39-40). The objectives' completions come as
Command messages. None of this text is in the archives (see above).

### Positions

Messages give map positions in the Tricorder's units: the energy source
at <332:152>, the mine at 404, 220, and the cursor's terrain readout near
the colony at the start <407:209>. These fit 16 units per block (1024 across
F1's 64 blocks), with the second number counted from the other edge than
the grid's row 0 ([gmf.md](gmf.md)): the mine is then at block (25.25,
50.25), inside F1's only large deposit cluster (blocks 16-35, rows 40-54),
on a kind-5 patch, and the colony right beside it, as the briefing's
"Place a Mining Station here" says.

### Building and fighting

- **Construction** (the vehicle yard, 6:40-7:22): the yellow-striped
  apron appears first, on its own; then the building rises out of the
  ground (6:51 its lower walls, dark; 7:11 the arched hangar, marked "HBH",
  nearly full height) and the "Vehicle yard complete" message comes at 7:22:
  about 40 s from placing. A dark see-through rectangle at 5:50 looks like
  the placement outline. The construction yard took until 5:03, the power
  generator 5:49, the accommodation block 20:54.
- **Concrete:** the colony stands on a patchwork of concrete squares a
  tile each, from the start. F1's map has the plain flat ground model
  (number 6, the most common on the map) there and only scattered concrete
  tiles (C below), so the concrete isn't the map's own: either the colony
  paves the tiles it's built on, or flat tiles there are drawn with a
  concrete texture. Not known which. The map round the colony (blocks
  23-27, rows 48-52, four tiles each, model numbers mod 10):

  ```
  52  37h7 3300 7777 3303 0664
  51  6434 0077 4103 1644 6664
  50  4566 6446 4614 6661 5466
  49  6154 01C6 6666 6666 6465
  48  5514 2002 6666 6666 6546
  ```
- **Combat** (5:50, Klingon vehicles against the colony's): a white ring
  round a unit as its shield takes a hit (the `*ripple*` textures), yellow
  muzzle flashes, a thin blue phaser line. Selected units carry a row of
  small orange marks above them.

### Buildings and their models

Matched by comparing the recording with every model of `fedbuild.cat` (and
`fedactor.cat`) drawn in the data viewer. The names come from the selection
panel where a building was selected, or from the "complete" messages.

| In the recording | Model | Sure? |
|---|---|---|
| Colony hub: the octagonal tower the mission starts with (selected 12:30, "COLONY HUB") | `fch`; three round pods (`fch_b`) by 9:48; a wide flat octagon (`fch_c`) by 14:28 | yes (selected; shapes) |
| Vehicle yard (built 6:40-7:22, selected 9:27): an arched hangar marked "HBH" | `fvpy_a` (with its apron), then `fvpy_b`, `fvpy_c` beside it: one section at 7:22, two by 15:08, three by 24:28 | yes |
| Resource processing station: a disc with a glowing cyan centre and arms with red and green lights | `frps_a` with `frps_b`/`frps_c` arms: four by 8:48, six by 22:37 | yes (shape) |
| A hexagonal block with a ringed dome, a stepped base with round grilles, a box in front and an apron (6:28, 9:29) | `fss_a`; by 26:28 with four wings round it (`fss_b`, the wall pieces) | yes (shape); its name isn't known |
| A tall chamfered box with a ringed dome, yellow-black corner stripes and painted numbers ("1", "4") | `prostor` (the model's own texture shows a "2") | yes (shape) |
| Construction yard (complete 5:03): a low two-step box with an apron | `fcy` | likely |
| Accommodation block (complete 20:54): a curved shell | `accom` | likely |
| Security center (selected 21:40): a low flat building with a raised middle and an apron | `fsc` | likely |
| A building with a wavy roof (about 24:08) | `fhf` (hydroponics farm) | likely |

So the `_a`/`_b`/`_c` families and the extra pieces (`fch_b`, `fch_c`,
`fss_b`) aren't separate buildings or damage states: **a building is
upgraded by adding pieces round its first model**: sections to the vehicle
yard, arms to the processing station, pods and then a wider base to the
hub, wings to `fss_a`. The `_d` models look like their plain ones; the
`_p` ones are hollow shells, perhaps the walls a building rises with while
it's built (not confirmed).

Textures seen: `fss_a`'s side panels, which the viewer can't place (its
two unknown texture numbers, drawn red and olive), are grey with two blue
bands in the game, like the panels of the `fss` texture. `prostor`'s
corners carry different painted numbers per building.

Not identified: the power generator (`fpgs`, complete 5:49), the mining
stations (`fms`, only seen from afar on the hills), the defences (`fpdt`,
`fpds`, `fsg`), `landpad`, `foodstor` and the sickbay (`fsb`), which must
exist (research is done there at 26:00).

The placement outline is a see-through box over the tiles, green where the
building can go (17:09) and red where it can't (22:37).

### End

31:19 the last Command message; the emblem with the MISSION REPORT over it
from 31:27; the recording ends at 32:21.

## Still to look at

The units' speeds (the camera rarely holds still long enough), the
buildings not identified above, the animation rates, and the messages
missed (5-6, 14, 16, 23-24).
