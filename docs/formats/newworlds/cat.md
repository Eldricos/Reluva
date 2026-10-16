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
| 0x08 | u32 | 10218 / 107372 | data base: stored file *i* starts at this value + its offset (checked for entries 0–2 of `rommisc.cat`) |
| 0x0c | u32 | 31 / 342 | probably the entry count |

## Table of contents

Entries start at 0x10 and are 334 bytes apart (checked for entries 0–2 of
`rommisc.cat`):

| Entry offset | Type | Meaning |
|---|---|---|
| +0x000 | u32 | data offset; each one is the previous offset + previous size + 6 |
| +0x004 | u16 | entry index (0, 1, 2…) |
| +0x006 | u8 | 0 |
| +0x007 | u8 | same for every entry in an archive (`0x15` in `rommisc.cat`, `0x17` in `glblgame.cat`); possibly an archive ID |
| +0x008 | char[260] | original build-machine path, NUL-terminated (260 = Windows `MAX_PATH`), or empty for some entries (e.g. `rommisc.cat` entry 18, `glblgame.cat` entry 20). Bytes after the NUL are uncleared memory from the packing tool, not data |
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

## Stored files

Each stored file starts at `base + offset` with a 6-byte prefix: u16 tag
(`0x32` for the three `.stb` files checked) and u32 size (equal to the
entry's metadata size), followed by the data. The next entry's offset is
this offset + size + 6.

## Stored file types seen so far

From the stored paths: `.stb` (string tables, likely UTF-16: leftover UTF-16
English text appears in the path buffers), `.lbp` (textures, under `lbp\txr`),
`.mmb`, `.mp0`, `.geo` (geometry, under `geo`), `.wav`.

## Open questions

- What the header fields at 0x00 and 0x06 mean, and the prefix tags.
- How 342 entries of 334 bytes fit before `glblgame.cat`'s data base
  (107372): either some entries are shorter, or the layout differs.
