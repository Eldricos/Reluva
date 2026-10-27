# Star Trek: New Worlds: a second channel's first play (tutorials, K1, F1, R1)

A 2 h 05 min stream recording by another channel ("14 Degree East",
YouTube `ytohvP4iACU`; 1280×720 at 60 fps, the 640×480 game scaled 1.5×
to x 322-1280, a face camera and the box art on the left). Observed
behaviour, as in the other play pages; times are the recording's
(h:mm:ss). The messages' wording isn't copied: the tables give their gist.
October 2026.

| Time | What |
|---|---|
| 0:00-0:12 | the intro movie, the main menu and the campaign screen |
| 0:12-1:14 | the tutorials, several of them, with their mission reports |
| 1:15-1:17 | a break (the stream's own adverts) |
| 1:18-1:23 | K1 started (its briefing played), then left by the player |
| 1:25-1:41 | K1 again, briefing skipped, played for 16 minutes (until the screen goes black at 1:41:17) |
| 1:43-1:46 | F1 started, played briefly |
| 1:46-1:58 | R1: briefing, then 7½ minutes of play |
| 1:59-2:04 | the main menu, then a black screen to the end |

The player never turned the camera from the isometric view, and never
opened COMMUNICATIONS to step back through messages.

## The compass

Every mission starts with the strip reading `---350---+--[ N ]--+---010--`
(F1 at 1:44:02, R1 at 1:51:09; K1's strip is the same once the play
view shows): the starting camera looks north, and the bearings rise to the
right (clockwise, as a compass). The strip is one character a degree,
about 13 each way of the heading: a `+` every 5 degrees, a three-digit
label every 10, the quarter in brackets, dashes between. Since no one
turned the camera, whether the brackets always mark the centre or only
frame N, E, S and W isn't seen. Reluva draws it this way now.

With the data's rows growing northward ([gmf.md](gmf.md#map-positions-in-the-game)),
that settles which way row 0 faces: south, at the bottom of the starting view.

## K1

Played 1:25:29-1:41:17.

The counter's numbers are this play's own: the order depends on what the
player builds and finds first, so they don't match the other recording's
([play-k1.md](play-k1.md)).

| Time | # | From | Gist |
|---|---|---|---|
| 1:25:31 | 1 | Operations | Taal welcomes the commander; an honour to serve their house |
| 1:26:17 | 2 | Engineering | construction yard finished |
| 1:27:01 | 3 | Engineering | power generator finished |
| 1:27:55 | 4 | Engineering | first colony hub upgrade finished |
| 1:28:31 | 5 | (not seen: arrived with 6) | |
| 1:28:31 | 6 | Incoming message (a Taubat) | the intruders defile their holy sites and will be driven off |
| 1:28:33 | 7 | Engineering | resource processor finished |
| 1:28:55 | 8 | Operations | this can't be borne: they must be destroyed |
| 1:28:59 | 9 | Intelligence | a Taubat lookout post at <271:89> |
| 1:29:11 | 10 | Engineering | first construction yard upgrade finished |
| 1:30:45 | 11 | Engineering | vehicle yard finished |
| 1:31:23 | 12 | (not seen: arrived with 13) | |
| 1:31:23 | 13 | Engineering | every building needed is up |
| 1:39:21 | 14 | Intelligence | a Taubat lookout post at <242:149> |
| 1:40:09 | 15 | Engineering | the mine at 220, 140 has run dry |

Against Reluva's K1 script (rewritten from the game's mission graph):

- **5 and 6 together**: finding any Taubat site has Intelligence name it
  and sets off the Taubat's message at once. The two lookout posts are 9
  and 14, so 5 was the first site found (the fortress, temple or vehicle
  factory). The other recording's unseen pairs fit the same way.
- **8**, Operations' answer, comes 24 s after the Taubat's message: the
  script's 30-unit timer at 0.8 s a script second
  ([mission-scripts.md](mission-scripts.md)).
- **12 and 13 together**: "every building needed is up" (objective 15)
  fires when the last of the five finishes, beside that building's own
  "finished" message; with the construction yard, generator, processor and
  vehicle yard already reported, 12 is the mining station's.
- The generic lines (each building finished, the first hub and
  construction yard upgrades, a mine run dry) are the ones the other
  recording's unseen numbers stood for.

## R1

Played 1:51:00-1:58:30.

| Time | # | From | Gist |
|---|---|---|---|
| 1:51:00 | 1 | Operations | greetings from the command staff, who trust the commander fully |
| 1:53:43 | 2 | Engineering | construction yard finished |
| about 1:54:55 | 3 | Engineering | vehicle yard finished |
| about 1:55:15 | 4 | Intelligence | the three rival Romulan cloaked tanks are destroyed |
| 1:56:08 | 5 | Operations | colony power too low, systems shutting down; build generators |
| 1:57:09 | 6 | Operations | the same |
| 1:58:09 | 7 | Operations | the same |

The power warning repeats every 60-61 s while power stays short, as
Reluva's economy has it (`kWarnEvery`, 60 s). So in the other R1
recording ([play-r1.md](play-r1.md)), the unseen messages could as well be
repeats as other lines; there the first warning on screen was 8, at 12:16.

## Not answered here

- **Build times**: the sites go up while the camera is elsewhere, so no
  order-to-finish time is certain. R1's construction yard took between 48
  and 74 s (its build menu opened at 1:52:30, the site stood by 1:52:56,
  finished 1:53:43).
- **The Tricorder's mineral rule**: the readouts seen are TERRAIN AT,
  RELIEF X2, IMPASSABLE and FRIENDLY UNIT, never an ore's name.
- R1's briefing shows its star system at 1:47:30, but R1 isn't among the
  missions whose star is asked about.
