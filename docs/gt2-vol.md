# GT2.VOL

`GT2.VOL`, in the disc root, holds all of the game's assets except the music (`MUSIC.DAT`) and the code (`SCUS_944.88`, `GT2.OVL`). Both discs use the same format. On the NTSC-U 1.2 Simulation disc it is 488 MB and holds 11578 files in 21 directories; on the Arcade disc, 10622 files.

## Extracting

```bash
gt2-discinfo vol ls "Simulation.cue"
gt2-discinfo vol ls "Simulation.cue" carparam
gt2-discinfo vol extract "Simulation.cue" E:/re/gt2/vol-sim
gt2-discinfo vol extract "Simulation.cue" E:/re/gt2/vol-sim sound/sys.ins
```

`ls` prints the file table index, byte offset in `GT2.VOL`, size and path of each file. The optional VOL path selects one file or everything under a directory, with or without the leading `/`. In Git Bash, leave the `/` out: MSYS rewrites arguments starting with `/` into Windows paths.

Files are written as stored. Many are gzip streams (`.gz`, but not only). A few are containers holding several gzip streams back to back with zero padding, e.g. `/gtmenu/*/gtmenudat.dat` and `/arcade/arc_topmenu*`.

Like every other output of the tool, extracted files are game data: keep them outside the repository.

## Format

All values are little-endian.

### Header and file table

The first 0xc000 bytes:

| Offset | Type | Contents |
|---|---|---|
| `0x00` | `char[4]` | `GTFS` |
| `0x04` | `u32` | 0 |
| `0x08` | `u32` | `0x2d642d3d` on NTSC-U 1.2 (unknown) |
| `0x0c` | `u32` | 0 |
| `0x10` | `u32[]` | file table |

Each file table entry packs two values:

- `entry & ~0x7ff`: the file's byte offset in `GT2.VOL` (files start on a sector).
- `entry & 0x7ff`: the unused bytes at the end of the file's last sector.

So `size(i) = (table[i + 1] & ~0x7ff) - table[i]`. After the last file comes an entry equal to the size of `GT2.VOL`, then 0.

File 0 is the header itself, file 1 the directory.

### Directory

File 1 is an array of 0x20-byte records:

| Offset | Type | Contents |
|---|---|---|
| `0x00` | `u32` | modification time (Unix time, 1999) |
| `0x04` | `u16` | file: its file table index. Directory: the record number of its first entry |
| `0x06` | `u8` | flags: bit 0 = directory, bit 7 = last record of this directory |
| `0x07` | `char[25]` | name, NUL-terminated |

The root directory starts at record 0. Every other directory starts with a `..` record whose index is the parent's first record. Root-level data files have names starting with `.` (`.carinfoa`, `.usedcar`, …).

## In the game

Ghidra names from `SCUS_944.88` (Simulation):

| Address | Name | Role |
|---|---|---|
| `0x80011494` | `Disc_LocateDataFiles` | Boot: finds `GT2.VOL`, `MUSIC.DAT` and `STREAM.DAT` (absent on NTSC-U) with a small ISO9660 reader (`Iso_*`, state in `g_IsoCtx`) |
| `0x800102dc` | `Vol_Init` | Reads the first 0x8c000 bytes, keeps the header at `g_VolHeader` (`0x801e35f0`, table at `0x801e3600`) and file 1 (directory plus resident data) at `g_VolDir` (`0x800a97d0`) |
| `0x80010228` | `Vol_ResolveBootFiles` | Resolves 248 fixed paths (`g_VolBootNames`) to table indices in `g_VolBootIds` (`0x801e2ef0`) |
| `0x800100e4` | `Vol_FindEntry` | Path lookup in the directory |
| `0x8005d74c`, `0x8005d79c` | `Vol_GetSector`, `Vol_GetSize` | Table entry to sector and size; `VolId_*` variants take a `g_VolBootIds` slot |
| `0x8005d908`, `0x8005d92c` | `Vol_ReadFile`, `Vol_ReadFileAsync` | Read a whole file; sectors are relative to `g_VolLba` (`0x801c93e8`) |
| `0x80010000` | `Overlay_OpenFile` | Locates `GT2.OVL` and caches it in RAM when memory allows |

At boot the game also builds lookup tables from directory listings: car models from `/carobj` (`g_CarAssets`, searched by `CarAsset_Find`), logos from `/carlogo`, wheels from `/carwheel`, engine sounds from `/engine` and course maps from `/crsmap`.
