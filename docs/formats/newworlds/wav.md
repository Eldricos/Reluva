# Star Trek: New Worlds: `.wav` sounds

Status: **fully decoded**. Stored in the `.cat` archives with chunk tag
`0x33` under `stnwsfx/` (85 in `kliactor.cat`, 3 each in the alpine and
desert planet archives). Worked out from the files themselves, October 2026.
`reluva-tool wav-info <file.wav | archive.cat>` shows one or surveys them
all; `reluva-tool wav-decode <in.wav> <out.wav>` writes 16-bit PCM. All
decode.

## Layout

Ordinary RIFF WAVE files (little-endian):

```
"RIFF" u32 size "WAVE"
"fmt " u32 20:
    u16 format          0x11 (IMA ADPCM) in every file seen
    u16 channels        1
    u32 rate            22050 (one file, kneg1, 11025)
    u32 bytes per second
    u16 block size      512
    u16 bits per sample 4
    u16 2, u16 samples per block (1017)
"fact" u32 4: u32 frames
"data" u32 size: the blocks
```

Other chunks are skipped. The last block is usually short; the `fact` count
says where the sound really ends (a few samples before the block does).

## IMA ADPCM blocks

Each block starts, per channel, with s16 the first sample, u8 the step index
(0-88) and a u8 0. Then 4-bit codes, in groups of 4 bytes per channel
(8 samples), low nibble first. Each code updates the sample and step with
the standard IMA tables (89 steps, 7 to 32767; index changes −1, −1, −1, −1,
2, 4, 6, 8):

```
diff = step >> 3
if code & 4: diff += step
if code & 2: diff += step >> 1
if code & 1: diff += step >> 2
sample += code & 8 ? −diff : diff   (clamped to 16 bits)
index  += change[code & 7]          (clamped to 0-88)
```

A 512-byte mono block holds 1 + 508 × 2 = 1017 samples.

Checked against ffmpeg's decoder: the samples differ by at most 33, from
ffmpeg computing `diff` exactly rather than by the shifts above; lengths
differ by the few samples ffmpeg keeps past the `fact` count.

## Copies

The archives store some sounds many times over, each copy a full file of
its own. In `kliactor.cat`, 62 of the 85 are different: 13 general sounds
(weapons, shields, doors, each vehicle's start-up), then a set of 12 per
vehicle (`apc`, `fgs`, `mdb`, `pa`, `dt`, `sv`), 8 voice lines numbered 001,
002, 004 and 005 with two variants each, followed by four copies of the
short refusal `kneg1.wav`. Those four look like slots of a fixed set that
the vehicle has no line of its own for. The Federation archives reportedly
do the same with `fneg1.wav`, stored also as `default.wav`, `pass.wav` and
`fail.wav`. What each slot is for isn't known yet.

`reluva-tool wav-info <archive.cat>` marks each copy with the first one it
repeats.

## In Reluva

The data viewer's last page (**Tab**) lists the sounds with their waveform;
**Space** or **Enter** plays or stops one, **A** plays each as it's
selected. Copies are listed once, with their count and other names in the
title; **F** lists every copy.
