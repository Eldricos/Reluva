# Star Trek: New Worlds: `.cat` archives

Status: **structure understood; stored file formats not yet studied.**
Worked out from hex dumps and `reluva-tool cat-probe` runs on the clean
retail `rommisc.cat` and `glblgame.cat`, October 2026. Run
`reluva-tool cat-probe <file>` to check this description against any archive.

All values are little-endian.

## Chunks

The whole file is a tree of chunks. Every chunk is:

| Offset | Type | Meaning |
|---|---|---|
| 0 | u16 | tag |
| 2 | u32 | payload size (excluding these 6 bytes) |
| 6 | … | payload |

Tags seen so far:

| Tag | Meaning | Evidence |
|---|---|---|
| `0x01` | root: the whole file | size = file size − 6 in both archives |
| `0x05` | table of contents | its size accounts exactly for the entries (below) |
| `0x16` | `.lbp`, `.mp0`, `.eb0` data (textures) | from entry names |
| `0x29` | folder: payload is the chunks of the following entries | `rommisc.cat` entry 18: size 9256 = (6 + 8) + (6 + 9236), its two children |
| `0x2a` | `.mmb` data (8 bytes in both cases seen) | from entry names |
| `0x32` | `.stb` data (string tables) | from entry names |
| `0x33` | `.wav` data | payload starts with `RIFF`, and the RIFF size + 8 = the chunk size (54,844) |

## File layout

```
0x00  chunk tag 1 (root), size = file size - 6
0x06    chunk tag 5 (table of contents), size T
0x0c      u32 entry count
0x10      entries, one after another (file 334 bytes, folder 264 bytes)
0x0c+T  item chunks, in entry order; folders contain their children's chunks
```

Check: `rommisc.cat` has 29 files and 2 folders: 29 × 334 + 2 × 264 + 4 =
10218 = T. For `glblgame.cat`, T = 107372 with 342 entries works out to
exactly 244 files and 98 folders.

## Table entries

| Entry offset | Type | Meaning |
|---|---|---|
| +0x000 | u32 | offset: the item's chunk is at file position offset + T. Equivalently, offsets are positions in the file as if the table's contents were removed: the first is 12, right after the two chunk headers |
| +0x004 | u16 | index. Increases by 1 per entry, except that each folder uses one extra index after its last child (as if an unstored "end of folder" entry existed): in `rommisc.cat`, folder 18 holds 19 and 20, and the next entry is 22 |
| +0x006 | u8 | 0 |
| +0x007 | u8 | same in every entry of an archive (`0x15` in `rommisc.cat`, `0x17` in `glblgame.cat`); possibly an archive ID |

Folder entries end there plus 256 bytes (264 total); their path is empty.
File entries continue:

| Entry offset | Type | Meaning |
|---|---|---|
| +0x008 | char[260] | original build-machine path, NUL-terminated (260 = Windows `MAX_PATH`). Bytes after the NUL are uncleared memory from the packing tool |
| +0x110 | u16 | mode, e.g. `0x8124` = regular file, read-only (apparently a C `stat` copy) |
| +0x112 | u16 | link count (1) |
| +0x11e | u32 | file size (equals the chunk's payload size) |
| +0x122 | u32 | access time (Unix time, date only, as FAT stores it) |
| +0x126 | u32 | modification time (Unix time) |
| +0x12a | u32 | creation time (Unix time) |
| +0x12e | u32 | modification time again |
| +0x132 | u32 | Windows file attributes (`0x20` archive, `0x21` archive + read-only) |
| +0x142 | 12 bytes | unknown; values look random across entries, so probably more uncleared memory |

Example (`rommisc.cat` entry 0): offset 12 → chunk at 10230, tag `0x32`,
size 3168, modified 2000-08-14 23:14 UTC, path
`c:\prjs\game\stc\bin\rsc\dat\pc_win32\str\eng\miss\rom01.stb`.

## Stored file types seen so far

From the stored paths: `.stb` (string tables; leftover UTF-16 English text in
the path buffers suggests UTF-16), `.lbp` (textures, under `lbp\txr`), `.mmb`
+ `.mp0` and `.mmb` + `.eb0` pairs grouped in folders (possibly mipmapped and
environment-map textures), `.geo` (geometry), `.wav`.

## Open questions

- The meaning of the u8 at entry +0x007.
- The formats of the stored files themselves.
