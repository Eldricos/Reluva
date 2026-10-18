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

MP3s named after the mission and a number from 005 (`F1005`, `K12009`,
`J1R011`; joint missions carry the side's letter: `J1F`, `J1K`, `J1R`):

| Folder | Missions | Files |
|---|---|---|
| `FedVO` | F1 (005-015), F2 (005-012), F3 (005-017), F8 (005-014), F12 (005-010), F14 (005-013); J1F (005, 006, 010-021), J2F-J9F | 135 |
| `KlingVO` | K1 (005-013), K2 (005-012), K7 (005-013), K8 (005-015), K12 (005-013, plus a `K12009A`), K14 (005-011); J1K-J9K | 120 |
| `RomVO` | R1 (005-013), R2 (005-016, no 012), R8 (005-014), R9 (005-014), R12 (005-009), R14 (005-010); J1R-J9R | 116 |

Exactly the missions with text in the side's misc archive (`fedmisc`,
`klimisc`, `rommisc`; no J5), mission 14 with its own recordings though it
reuses 12's map.

**The recordings are speeches, not the strings read aloud.** Heard
(October 2026): `F1005` (34 s) opens the briefing, "I wish I could be there
to brief you in person but the…"; `F1012` (6 s) "This mineral field should
supply complete resources…"; `F1013` (11 s) "While the Taubat government
is desperate to involve the Federation…"; `F1015` (10 s) "The situation
couldn't be much worse, you'll be caught between the greed…". String 12 of
`fed01.stb` is the mission-pass page and 13 the fail page, so the numbers
aren't string ids, though every file's number happens to be one: they're
the mission's speech clips in order, from 005 (the opening briefing),
played at points the mission's script decides (which isn't in the data
files; presumably `STNW.exe`). That's why their sizes don't follow the
strings' lengths.

**Format:** MPEG-2 Layer III, 64 kbit/s, 22,050 Hz, mono, with an ID3v2
tag naming the original file (`F1005.WAV`) and the encoder (SoundJam MP).

## Announcements (`Misc`)

WAV files, the same for every mission:

- **Building complete** (`com`), one per building type and side:
  Klingon `A_k…com` (17: `ka` accommodation, `kams`, `kapg`, `kch` colony
  hub, `kcy` construction yard, `kdt`, `klf` livestock farm, `kms` mining
  station, `kpg` power generator, `kpht` photon artillery, `krp` resource
  processor, `ksb` sickbay, `ksf`, `ksg` static gun, `kss` science
  station, `ktp` transporter pad, `kvy` vehicle yard), Federation
  `B_f…com` (17: `fab`, `fams`, `fapg`, `fcy`, `fhf` hydroponics farm,
  `fms`, `fpg`, `fpht`, `fpt`, `frp`, `fsb`, `fsby`, `fsc`, `fsf`, `fsg`,
  `ftp`, `fvy`), Romulan `B_r…com` (10). `fed01.stb`'s "Vehicle yard
  completed, Sir." is such a message.
- **Events**, per side (`f`, `k`, `r`): `atckvoc` (under attack),
  `lowpowr` (low power), `miswin` / `mislose` (mission won / lost),
  `objmet` (objective met).
- **Eight lines per side**, each in one voice (`FED001c` … `FED008a`,
  `KLI001a` … `KLI008b`, `ROM001b` … `ROM008b`): numbered and lettered like
  the vehicles' voice lines in the actor archives (line, then voice a-e),
  so probably generic unit responses.
- **Tutorials:** `Tutor01`-`tutor41` and `tut-2_01`-`tut-2_27` (MP3), the
  training and tutorial missions' speech (`cvr03.stb`'s `~INSTRUCTION~`
  pages).

## Music (`Music`)

MP3s:

- **A theme per planet type**, about 2.9 MB each: `AlpineStart`
  (`alpwrld`), `DesertStart` (`deswrld`), `IceStart` (`icewrld`),
  `LavaStart` (`vlcwrld`), `MarsStart` (`redwrld`, the red / barren worlds)
  and `MountainStart` (by elimination `alnwrld`).
- **A theme per race met:** `Federation_`, `Klingon_`, `Romulan_`,
  `Hubrin_`, `Metar_`, `Taubat_Encounter`.
- **Stings per side** (Federation, Klingon, Romulan): big danger, colony
  in danger, general danger, mission failed, mission succeeded, objective
  met, mystery; and `birdofprey`, `RomBirdofPrey`, `fedcrashedss`.

`StarTrek.wav` (2.9 MB, in `Media/` itself) is presumably the main menu's
music.

## In Reluva

The shared audio code (`src/common/audio/`, for every engine) decodes both
formats: `decodeSound` picks WAV (PCM, IMA ADPCM) or MP3 (through minimp3,
`third_party/minimp3`) by content, and `Mixer` (`mixer.h`) plays any number of decoded sounds at
once: resampled to the output rate, each with its volume, pan and looping,
and music, speech and effects volumes for the game's settings. The data
viewer's sounds page lists the
game folder's loose `.mp3` and `.wav` files after the archives' sounds and
plays them; `reluva-tool wav-info` and `wav-decode` take MP3s too. The
missions page plays each mission's voice clips and its planet's theme (see
[level.md](level.md#in-reluva)). The voice
MP3s decode to within 1 of ffmpeg's samples.

## Open questions

- When each mission's speeches play (the scripts aren't in the data).
- Which of the eight lines per side is what.
