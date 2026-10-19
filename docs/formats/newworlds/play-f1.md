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

## Still to look at

The recording covers the whole mission (the colony growing, the scans, the
temple's capture); not yet gone through: the build order and times, unit
speeds, what each building looks like when finished against the models, and
the animation rates.
