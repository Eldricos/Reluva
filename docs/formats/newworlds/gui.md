# Star Trek: New Worlds: the GUI archives

`fedgui.cat`, `kligui.cat`, `romgui.cat` (one per side) and `glblgui.cat`
(shared). Checked October 2026: they hold only pictures (`.lbp`, chunk tag
`0x16`; all 748 decode, see [lbp.md](lbp.md)), fonts ([fif.md](fif.md)) and,
in `glblgui`, sounds and `frend.stb`. No interface layouts: where the
pictures go on screen isn't in any archive (presumably `STNW.exe`).

| Archive | Pictures | Fonts |
|---|---|---|
| `fedgui` | 241 | `fepnttle`, `fe8pt`, `fe5pt` (×5) |
| `kligui` | 197 | `klpnttle`, `kl8pt`, `kl5pt` |
| `romgui` | 197 | `rmpnttle`, `rm8pt`, `rm5pt` |
| `glblgui` | 113 | `fedengft`, `feengft2`, `final12` |

## Pictures

Under `lbp/gui/<side>/`: buttons, frames and corners of the interface
(`corner`, `asybcrnr`, `opacbutt`, `ops2butt`, `basebut`, `vcr`, …), and
**portraits** under `ptrt/`, each small (`sm/`) and large (`lg/`, 64 × 60):

| Folder | What | Federation examples |
|---|---|---|
| `build` | buildings | `fhub`, `fca` (accommodation), `flp` (landing pad), `fcy`, `fvpy`, `frps`, `fss`, `fsb`, `fms1`, `fms2`, `fpgs1`, `fpgs2`, `fpds`, `fpdt`, `fsc`, `fsg`, `fhf`, `fsfr`, `f_hcp` |
| `upgde` | building upgrades | `frps_bs`-`frps_fs`, `fss_bs`-`fss_fs`, `fsb_bs`-`fsb_es`, `fhub_bs` |
| `craft` | vehicles | `fapc`, `fgs`, `fmsg`, `fpv`, `fsv` (the vehicles with voice sets, see [wav.md](wav.md)), `fconb` (the construction vehicle), `fwb`, `fcrgb`, `fptl`, `fus`, `fs`, … |
| `crew` | crew and races | `f_c1`-`f_c34`, `fairbk` (Admiral Fairbanks), `f_sarek`, `f_fox`; on every side `taubat`, `metar`, `hubrin`, `orion` |

The Klingon and Romulan sets are alike (`kchub`, `k_kirgat`, `k_plyr`;
`rchub`, `r_narok`, `jathek`, `r_plyr`). The portraits link the names:
`fhub` is the `fch` model, `fca` `accom`, `flp` `landpad`; a building's
upgrades are its `_b`, `_c`, … models (the resource processor's five,
`frps_bs`-`frps_fs`, match the tutorial's "upgrade it to include all 6 ore
processors").

The building portraits are rendered from the models with all their
textures, so they show what the files leave out: the Federation buildings'
yellow-and-black hazard-striped aprons, which identified the `pad` texture
(see [geo.md](geo.md#textures)).

`fedgui` and `glblgui` also hold 21 test pictures under another build path,
`game/stc/bin/rsc/res/kurtscou/` (`kscout1s`-`5l`, `ktank1s`-`5l`,
`ksmile_b`, `ksmile_m`).

## Emblem screens

`glblgui.cat` holds each side's emblem as a full-screen picture cut into six
tiles, `ufp001`-`006` (the Federation), `kliemp01`-`06` (the Klingon Empire)
and `romemp01`-`06` (the Romulan Star Empire): three across and two down,
256 + 256 + 128 by 256 + 224 pixels, so together the 640 × 480 screen. They
are the emblem screen that ends a briefing (seen in recordings of play,
[play.md](play.md)). The `f_win`, `k_win`, `r_win` and `f_lose`... sets
beside them are likewise six tiles each, presumably the mission won and
lost screens.

