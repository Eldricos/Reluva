# Star Trek: New Worlds: `.cat` archives

Status: **partly understood.** Worked out from hex dumps of the clean retail
`rommisc.cat` and `glblgame.cat` (first 1 KB, last 512 bytes, `strings`
output), October 2026. Use `reluva-tool cat-probe <file>` to test the layout
below against a real file.

All values are little-endian.

## Header

| Offset | Type | Value seen | Meaning |
|---|---|---|---|
| 0x00 | u16 | 1 | unknown (version or chunk type) |
| 0x02 | u32 | file size − 6 | size of everything after these 6 bytes; exact in both files |
| 0x06 | u16 | 5 | unknown, same in both files |
| 0x08 | u32 | 10218 / 107372 | unknown |
| 0x0c | u32 | 31 / 342 | probably the entry count |

## Table of contents

Entries start at 0x10 and are 334 bytes apart (checked for entries 0–2 of
`rommisc.cat`):

| Entry offset | Type | Meaning |
|---|---|---|
| +0x000 | u32 | data offset; each one is the previous offset + previous size + 6 |
| +0x004 | u16 | entry index (0, 1, 2…) |
| +0x006 | u8 | 0 |
| +0x007 | u8 | type: `0x15` for `.stb` files, `0x17` for the `.lbp` seen in `glblgame.cat` |
| +0x008 | char[260] | original build-machine path, NUL-terminated (260 = Windows `MAX_PATH`). Bytes after the NUL are uncleared memory from the packing tool, not data |
| +0x10c | … | copy of the source file's metadata, apparently a C `stat` result: |
| +0x110 | u16 | mode, e.g. `0x8124` = regular file, read-only |
| +0x112 | u16 | link count (1) |
| +0x11e | u32 | file size |
| +0x122 | u32 | access time (Unix time, date only, as FAT stores it) |
| +0x126 | u32 | modification time (Unix time) |
| +0x12a | u32 | creation time (Unix time) |
| +0x12e | u32 | modification time again |
| +0x132 | u32 | `0x20` = Windows "archive" file attribute |

Example (`rommisc.cat` entry 0): offset 12, type 0x15, size 3168, modified
2000-08-14 23:14 UTC, path `c:\prjs\game\stc\bin\rsc\dat\pc_win32\str\eng\miss\rom01.stb`.

## Stored file types seen so far

From the stored paths: `.stb` (string tables, likely UTF-16: leftover UTF-16
English text appears in the path buffers), `.lbp` (textures, under `lbp\txr`),
`.mmb`, `.mp0`, `.geo` (geometry, under `geo`), `.wav`.

## Open questions

- What the header fields at 0x00, 0x06 and 0x08 mean.
- What the data offsets are relative to, and what the 6 bytes before each
  stored file contain.
- Whether all entries are 334 bytes (only 29 file names appear in
  `rommisc.cat` but the count field says 31).
