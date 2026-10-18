# Star Trek: New Worlds: `Media/`

Loose files in the game folder, in standard formats: MP3 (decoded by the
Miles Sound System's `.asi` plugin) and WAV. From a directory listing,
October 2026; the files themselves haven't been examined yet.

```
Media/
  StarTrek.wav     2,968,536 bytes (changed by the fan patch)
  FedVO/  KlingVO/  RomVO/  Misc/  Music/
```

## Voice-overs (`FedVO`, `KlingVO`, `RomVO`)

`FedVO` holds 135 MP3s named after the mission and a number from 005:

| Missions | Files |
|---|---|
| F1 | `F1005`-`F1015` (11) |
| F2 | `F2005`-`F2012` (8) |
| F3 | `F3005`-`F3017` (13) |
| F8 | `F8005`-`F8014` (10) |
| F12 | `F12005`-`F12010` (6) |
| F14 | `F14005`-`F14013` (9) |
| J1 (Federation side) | `J1F005`, `J1F006`, `J1F010`-`J1F021` (14; no 007-009) |
| J2, J3, J4, J6, J7, J8, J9 | `J<n>F005` up to `J<n>F009`-`J<n>F014` |

So the missions with Federation voice are those in `level.dat` for which
`fedmisc.cat` has text (F1, F2, F3, F8, F12, F14 and the joint missions;
there's no J5), and F14 has its own recordings though it reuses F12's map.
The joint missions' files carry the side's letter (`J1F`), presumably
`J1K` and `J1R` in the other folders.

Every file's number is an id in its mission's string table (`fed01.stb`
has ids 5-15 for `F1005`-`F1015`; `jnt01fed.stb` 5-21), but the files'
sizes don't follow those strings' lengths (correlation 0.02: `F1013`, for
"Objectives incomplete", is 87,550 bytes, longer than several objectives'),
so whether each recording speaks that string isn't confirmed; the voice
may be fuller than the briefing text. The counts don't match the
`default.wav` slots in `fedmisc.cat` either (7 for fed01, 11 files here).

## Open questions

- The `KlingVO`, `RomVO`, `Misc` (sound effects and tutorial voice) and
  `Music` listings.
- What each recording says (its length, and whether it reads its string).
