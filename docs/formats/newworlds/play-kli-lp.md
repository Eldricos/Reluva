# Star Trek: New Worlds: a Klingon Let's Play (episodes 002-015)

Fourteen episodes of the same YouTube Let's Play as
[play-fed-lp.md](play-fed-lp.md), the Klingon campaign (1280×720 at 60
fps, commentary over it), about 12 hours. Observed behaviour, as in the
other play pages; times are each episode's (mm:ss). The messages' wording
isn't copied. October 2026. The reviewers' full notes (every message, with
times) stayed in the working folder; this page keeps what answers or
changes something.

| Episode | YouTube | Mission | Notes |
|---|---|---|---|
| 002 | `ha4AgEHNcNQ` | K1 The Insult | won at 67:44 |
| 003 | `R1JtC5V_YKE` | K2 The Green and the Gray | won at 55:51 |
| 004 | `HrppoMmGinU` | J1K Into the Arena | still running when the episode ends |
| 005 | `ptlQQKL1bgQ` | J2K Absent Friends | lost twice: the hub destroyed (23:15), then the staff not rescued (70:27) |
| 006 | `bmGcRsmec1Y` | J3K The Mask Slips | won |
| 007 | `YSRp5BTaSgY` | K7 Knowledge Is Power | lost at 14:14 (hub destroyed), replayed and won at 83:04 |
| 008 | `R5AU3EDZvEY` | J4K Cry Havoc | won at 31:47 |
| 009 | `rgVkxsrqqVs` | K8 Operation Retaliate | won at 54:28 |
| 010 | `wpCbQsKWh94` | J6K Keep Your Friends Close | lost at 30:47, replayed and won at 58:39 |
| 011 | `pepiGj4UPq0` | J7K The Jaws of Death | won at 38:25 |
| 012 | `jFfCWT_a-oA` | K12 Death Rattle | lost at 46:48 (the Metar destroy the device) |
| 013 | `SZscop8e_Gw` | J8K Checkmate (won at 16:23), then K14 The Finest Hour | K14 still running when the episode ends |
| 014 | `DrntkbguLj8` | J9K The Lost Tribes | won at 31:41, with the Klingons' movie |
| 015 | `ckO76LSo4ls` | C10 The Archer of Heaven | won at 55:55 |

The game crashed several times in these recordings (a runtime error box
from `STNW.exe`; 007, 008, 012, 013, 015), and play went on from the
player's saves. As in the Federation episodes, the player deletes messages
at once, so the counter counts the messages held.

## Build times

The player seldom watched a site, so most readings are brackets. The order
is the moment the hologram is placed (the stock drops by the cost in the
same second); finished is the model complete or the panel's OPERATIONAL.

| Building | Mission | Order to finished | Construction bees | How sure |
|---|---|---|---|---|
| construction yard | J7K | about 34 s (±2) | 4, no other site | fairly sure |
| construction yard | J1K | 25-34 s | 6, one site | fairly sure of the window |
| construction yard | K1 | 22 s or less | ? | upper bound |
| arsenal (security center) | K1 | about 70 s | 6 | fairly sure |
| sickbay | K7 | about 61 s | 7 | moderately sure |
| improved hub | J3K | about 46 s (30 s from the first pieces) | 8 | fairly sure |
| advanced hub | J3K | about 45 s | 8 | fairly sure |
| power generator | K2 | 25 s or less | 4 | upper bound |
| power generator | K8 | over 21 s; two in a row at most 46½ s | 4 | brackets |
| vehicle yard | J2K | 111-113 s | 4, shared with another site | low |
| science station | K7 | 76-171 s | 4 | loose |
| resource processor | J9K | 90 s or less | 4 | low |

Reluva now uses the fairly sure ones for the Klingons (construction yard
32 s, arsenal 70, sickbay 61, improved hub 46, advanced hub 45;
`economy.cpp`); the power generator's 23 s fits both its brackets, and the
vehicle yard's 37 s stays (the J2K reading counts two sites at once). No
defence was timed, nor any build with one bee.

The hub panel's CNSTRCTNBEE count is the colony's total (it doesn't fall
while the bees are out): 4 in most missions, 6 in J1K, 8 in J3K, 7 in K1
at first (falling to 4 by 55:00, cause not seen), and in K7 it rose from 4
to 7 when the player had three made at the vehicle yard.

## Starting colonies

The NEW WORLDS panel in the first second of play:

| Mission | Colony | Stock (dil / poly / thermo / trans / nit / dur) | Against the data |
|---|---|---|---|
| K1 | 22/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| K2 | 22/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| J1K | 25/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| J2K | 25/55 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| J3K | 24/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| K7 | 20/50 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |
| J4K | 47/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| K8 | 28/50 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |
| J6K | 47/90 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |
| J7K | 26/50 | 150 / 1000 / 2300 / 200 / 4000 / 10000 | the same |
| J8K | 41/50 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |
| K14 | 55/70 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |
| J9K | 35/50 | 500 / 1000 / 2300 / 500 / 4000 / 5000 | the same |
| C10 | 47/50 | 660 / 1160 / 2300 / 660 / 5600 / 6600 | topped up |

K12's first reading (7:39, 47/50, 800 / 1734 / 2147 / 962 / 23880 /
24000) came after the player's first orders; its dilithium is still the
data's 800, so it most likely isn't topped up.

Every topped-up start is a colony placed with a vehicle yard, and every
other one has none: the yard's top-up to 160 000 units of each material
but thermoconc ([exe.md](exe.md#stores)), which Reluva has. Reluva's
starting stocks match all fourteen.

**The colony sizes don't match yet.** Reluva's after five seconds, against
the game's: K7 24/50 (20/50), J3K 25/50 (24/50), J4K 43/50 (47/50), K8
24/50 (28/50), J6K 42/75 (47/90), J7K 33/50 (26/50), J8K 38/50 (41/50),
K14 50/60 (55/70). Both numbers (the people, the capacity) are off, in
both directions; a Reluva bug still to look into. K2's 90 in
[play-k2.md](play-k2.md) is a later reading: the capacity grows with the
buildings (here 50 at the start, 60 by 16:39, 70 by 22:55, 85 by 27:46).

## Mining stations

Every exhausted mine reported (about 35, in K1, K2, J1K, J2K, J3K, K7, K8,
J6K, J7K and C10) lies on a deposit of its mission's map, most with all
eight neighbouring blocks the same kind. Two players' mines in K1 and J7K
were placed on the Tab Tricorder's green hatching (DILITHIUM AT under the
cross). No mission here has a Klingon AI colony, so the question of the AI
colony's mine is still open (its placement in the data is Reluva's, on a
deposit).

J3K's 115, 180 isn't a block's middle (that would be 116); it reads 5
clearly, enlarged. The game may report a mine's own position rather than
its block's.

## Maps

C10 plays on **`coverd1`**, as the executable registers it, not `f2` as
[play-c10.md](play-c10.md) guessed: its three mines (276, 444 kelbonite;
412, 404 silicon; 340, 380 kelbonite) all lie on `coverd1`'s deposits, two
of them on none in `f2`. A C10 mining station's panel held talgonite,
kelbonite and dolamide together: one mine digs several kinds.

## J2K: the Hubrin destroy the Klingon hub

In the first play the hub was shot down at 23:15 with no Federation or
Romulan unit near: a small pale flier over the colony, selected as HUBRIN
(owner HUBRIN), and the hub's efficiency falling 100, 93, 75, 68, 60 %
over 90 s. So in J2 the Hubrin attack the Klingon colony, which most
likely answers J2R's question ([play-j2r.md](play-j2r.md)). In both plays
the Romulan hub fell first, then the Federation's.

J2K's transporter position reads <112X280>, as in J2F and J2R
([play-j2k.md](play-j2k.md) has 112:260).

## J7: the magenta marker

The magenta square on J7's Tricorder map, at the colony's west edge, is
the **Hubrin guide**: the Tab Tricorder reads NEUTRAL UNIT over it and the
selection panel shows HUBRIN, owner HUBRIN (011, 4:20). The Federation hub
fell at 20:15 with no Klingon force near it.

## Messages worth noting

- **K1** (002): the routine scan lines (Engineering's structures normal,
  Science's no anomalies) and "every building needed is up" at 26:06, as
  [play-14east.md](play-14east.md#k1) had them.
- **K2** (003): the allied Taubat base is at **<279:126>** (read three
  times; [play-k2.md](play-k2.md) had <279:26>). New against that page:
  the three routine scan lines (Engineering, Science, Medical), a tank's
  "disruptor tanks finished", "the required defences are complete", and
  the end's temple, vehicle facility and gun platforms destroyed.
- **J1K** (004) and **C10** (015): a vehicle-yard line, "vehicle
  manufacture delayed: not enough crew; assign crew, build accommodation
  and a transporter pad" (also K7). "Vehicle manufacture delayed: not
  enough resources" is a separate line (J2K, K7).
- **J3K** (006): Science's "scan of the weapon's target finished",
  Operations' line that the bird-of-prey will join the final assault, the
  Communications section relaying the *Ves Jach*, and the Romulan hub
  destroyed (81:05). The counter jumps 1 to 3 at 78:56 and 7 to 9 at
  85:24.
- **K7** (007): the player's own losses go to Operations, "colony hub
  destroyed, mission failed" (14:14, by Taubat tanks and Metar Bur
  fliers). Engineering names the defences "photon torpedo launchers"
  (first, second) and the science block's special weapons wing.
- **J4K** (008): the **construction yard** is among the recaptured
  buildings (its own message and objective; [play-j4k.md](play-j4k.md)
  lists three).
- **K8** (009): Operations' "our bird of prey will be with you shortly"
  (22:19), not in [play-k8.md](play-k8.md).
- **J6K** (010): the Federation hub destroyed before the capture fails the
  mission (the report's one line: hub destroyed, ambassador not
  captured), as Reluva's script has it. In the replay the player's own
  force destroyed the Romulan hub (51:16, a plain hub). A Science line
  "this is futile, get someone else" after repeated scans.
- **J7K** (011): "building upgrade delayed: not enough resources", the
  upgrade's own form of the shortage line.
- **K12** (012): the counter jumps 2 to 4 at 40:31 (the "device in place"
  line arrives with the transport line).
- **J9K** (014): the "scan lost, try again" line and the counter's jump 2
  to 4 at 31:28 (Science's study of the anomaly device never shown), as
  on the Federation side.
- **C10** (015): Operations' praise when the warbird falls, and the end's
  reassignment to Tangred IV, neither in [play-c10.md](play-c10.md).

## The resource-shortage line

In most missions here (K7, K8, J7K, J2K) it arrives a few seconds after an
order the stock can't pay, one message each, minutes apart. In K1 and J1K
the panel redrew the line on top every 5 s with the counter unchanged (K1
26:16-27:42, J1K 21:56-24:44), so something re-sends it on a 5 s cycle
without adding a message; the Federation play's repeats
([play-fed-lp.md](play-fed-lp.md#messages-worth-noting)) may be the same.
Which orders were waiting wasn't on screen.

## Not seen

A Klingon AI colony's mining station, an upgraded Romulan hub close up, a
defence or a one-bee build timed, R2's many-legged unit, the star in any
star-system view (K7's and K8's show small bodies, none glowing; J9K and
C10 have no star-system step), who destroyed the Federation hubs in J4K,
J6K and J7K.
